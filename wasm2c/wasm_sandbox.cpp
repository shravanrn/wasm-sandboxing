#include "wasm-rt-impl.h"
#include "wasm_sandbox.h"

#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string>

#define getSymbol(lhs, type, function) lhs = (type)(uintptr_t)dlsym(ret->lib, #function); \
if(!lhs) { \
	printf("WasmSandbox Dlsym " #function " failed: %s\n", dlerror()); \
	return nullptr; \
} do {} while(0)

thread_local WasmSandbox* WasmSandboxImpl::CurrThreadSandbox = 0;

WasmSandbox* WasmSandbox::createSandbox(const char* path)
{
	WasmSandbox* ret = new WasmSandbox();
	ret->lib = dlopen(path, RTLD_LAZY);

	if(!ret->lib)
	{
		printf("WasmSandbox Dlopen failed: %s\n", dlerror());
		return nullptr;
	}

	using voidvoidPtr = void(*)();
	voidvoidPtr wasm_init_module;
	getSymbol(wasm_init_module, voidvoidPtr, wasm_init_module);

	wasm_init_module();

	using intvoidPtr = uint32_t(*)();
	intvoidPtr wasm_is_LLVM_backend;
	getSymbol(wasm_is_LLVM_backend, intvoidPtr, wasm_is_LLVM_backend);

	ret->ExportPrefix = wasm_is_LLVM_backend()? "_E" : "_E_";

	using jmpbufpvoidPtr = jmp_buf*(*)();
	getSymbol(ret->wasm_get_setjmp_buff, jmpbufpvoidPtr, wasm_get_setjmp_buff);

	using mallocType = uint32_t(*)(size_t);
	ret->wasm_malloc = (mallocType) ret->symbolLookup("malloc");

	using freeType = void(*)(uint32_t);
	ret->wasm_free = (freeType) ret->symbolLookup("free");

	using registerFuncTypeType = uint32_t (*)(std::vector<wasm_rt_type_t> *, std::vector<wasm_rt_type_t> *);
	getSymbol(ret->wasm_rt_register_func_type_with_lists, registerFuncTypeType, wasm_rt_register_func_type_with_lists);

	using registerFuncType = uint32_t(*)(void(*)(), uint32_t);
	getSymbol(ret->wasm_rt_register_func, registerFuncType, wasm_rt_register_func);

	using unregisterFuncType = void (*)(uint32_t);
	getSymbol(ret->wasm_ret_unregister_func, unregisterFuncType, wasm_ret_unregister_func);

	using getRegisteredFuncType = void* (*)(uint32_t);
	getSymbol(ret->wasm_rt_get_registered_func, getRegisteredFuncType, wasm_rt_get_registered_func);

	using getCurrentIndirectType = uint32_t (*)();
	getSymbol(ret->wasm_get_current_indirect_call_num, getCurrentIndirectType, wasm_get_current_indirect_call_num);

	wasm_rt_memory_t** wasm_memory_st;
	getSymbol(wasm_memory_st, wasm_rt_memory_t**, Z_envZ_memory);
	
	ret->wasm_memory = (*wasm_memory_st)->data;
	ret->wasm_memory_size = (*wasm_memory_st)->size;
	return ret;
}

void* WasmSandbox::symbolLookup(const char* name)
{
	std::string exportName = ExportPrefix;
	exportName += name;
	void** symbolAddr = (void**)(dlsym(lib, exportName.c_str()));

	if(!symbolAddr || !(*symbolAddr))
	{
		printf("Dlsym %s failed: %s\n", name, dlerror());
		return nullptr;
	}

	return *symbolAddr;
}

WasmSandboxCallback* WasmSandbox::registerCallbackImpl(void* state, void(*callback)(), void(*callbackStub)(), std::vector<wasm_rt_type_t> params, std::vector<wasm_rt_type_t> results)
{
	uint32_t func_type = wasm_rt_register_func_type_with_lists(&params, &results);
	wasm_rt_anyfunc_t func = (wasm_rt_anyfunc_t)(void*)callbackStub;
	uint32_t callbackSlot = wasm_rt_register_func(func, func_type);

	{
		std::lock_guard<std::mutex> lockGuard (registeredCallbackLock);
		registerCallbackMap[callbackSlot] = (void*) callback;
		callbackStateMap[callbackSlot] = (void*) state;
	}

	WasmSandboxCallback* ret = new WasmSandboxCallback();
	ret->callbackSlot = callbackSlot;
	ret->originalCallback = (uintptr_t) callback;
	ret->callbackStub = (uintptr_t) callbackStub;
	return ret;
}

void WasmSandbox::unregisterCallback(WasmSandboxCallback* callback)
{
	if(!callback) {	return;	}

	{
		std::lock_guard<std::mutex> lockGuard (registeredCallbackLock);
		registerCallbackMap.erase(callback->callbackSlot);
		callbackStateMap.erase(callback->callbackSlot);
	}

	wasm_ret_unregister_func(callback->callbackSlot);
	delete callback;
}

void* WasmSandbox::getUnsandboxedFuncPointer(const void* p)
{
	uint32_t slotNumber = (uintptr_t) p;
	return wasm_rt_get_registered_func(slotNumber);
}

void* WasmSandbox::getUnsandboxedPointer(const void* p)
{
	#if defined(_M_X64) || defined(__x86_64__)

		uintptr_t pVal = ((uintptr_t) p) & 0xFFFFFFFF;
		if(pVal == 0)
		{
			return nullptr;
		}
		else if(pVal < wasm_memory_size)
		{
			uintptr_t wasm_memoryVal = ((uintptr_t)wasm_memory);
			uintptr_t ret = pVal + wasm_memoryVal;
			return (void*)ret;
		}
		else
		{
			printf("WasmSandbox::getUnsandboxedPointer called with invalid value\n");
			abort();
		}

	#else
		#error Unsupported Platform!
	#endif
}

void* WasmSandbox::getSandboxedPointer(const void* p)
{
	#if defined(_M_X64) || defined(__x86_64__)

		//Wasm memory should be fixed at 4GB
		uintptr_t pVal = ((uintptr_t) p);
		uintptr_t wasm_memoryVal = ((uintptr_t)wasm_memory);
		if(pVal == 0)
		{
			return nullptr;
		}
		else if(pVal == wasm_memoryVal)
		{
			printf("WasmSandbox::getSandboxedPointer called with the sentinel null location.\n");
			abort();
		}
		else if(pVal > wasm_memoryVal && (pVal - wasm_memoryVal < wasm_memory_size))
		{
			uintptr_t ret = pVal - wasm_memoryVal;
			return (void*) ret;
		}
		else
		{
			printf("WasmSandbox::getSandboxedPointer called with invalid value\n");
			abort();
		}

	#else
		#error Unsupported Platform!
	#endif
}

int WasmSandbox::isAddressInNonSandboxMemoryOrNull(const void* p)
{
	void* wasm_memory_end = (void*) (((uintptr_t)wasm_memory) + wasm_memory_size);
	return p == nullptr || p < wasm_memory || p >= wasm_memory_end;
}

int WasmSandbox::isAddressInSandboxMemoryOrNull(const void* p)
{
	void* wasm_memory_end = (void*) (((uintptr_t)wasm_memory) + wasm_memory_size);
	return p == nullptr || !(p < wasm_memory || p >= wasm_memory_end);
}

void* WasmSandbox::mallocInSandbox(size_t size)
{
	uintptr_t ret = invokeFunction(wasm_malloc, size);
	return getUnsandboxedPointer((void*)ret);
}

void WasmSandbox::freeInSandbox(void* ptr)
{
	uint32_t ret = (uint32_t) (uintptr_t) getSandboxedPointer(ptr);
	invokeFunction(wasm_free, ret);
}

size_t WasmSandbox::getTotalMemory()
{
	return wasm_memory_size;
}