#include "wasm_sandbox.h"
#include "wasm-rt-impl.h"

#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string>

#define getSymbol(lhs, type, function) lhs = (type)(uintptr_t)dlsym(ret->lib, #function); \
if(!lhs) { \
	printf("WasmSandbox Dlsym " #function " failed: %s\n", dlerror()); \
	return nullptr; \
} do {} while(0)

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

	using jmpbufpvoidPtr = jmp_buf*(*)();
	getSymbol(ret->wasm_get_setjmp_buff, jmpbufpvoidPtr, wasm_get_setjmp_buff);

	wasm_init_module();

	using mallocType = uint32_t(*)(size_t);
	ret->wasm_malloc = (mallocType) ret->symbolLookup("malloc");

	using freeType = void(*)(uint32_t);
	ret->wasm_free = (freeType) ret->symbolLookup("free");

	wasm_rt_memory_t** wasm_memory_st;
	getSymbol(wasm_memory_st, wasm_rt_memory_t**, Z_envZ_memory);

	ret->wasm_memory = (*wasm_memory_st)->data;
	ret->wasm_memory_size = (*wasm_memory_st)->size;
	return ret;
}

void* WasmSandbox::symbolLookup(const char* name)
{
	std::string exportName = "_E_";
	exportName += name;
	void** symbolAddr = (void**)(dlsym(lib, exportName.c_str()));

	if(!symbolAddr || !(*symbolAddr))
	{
		printf("Dlsym %s failed: %s\n", name, dlerror());
		return nullptr;
	}

	return *symbolAddr;
}

void* WasmSandbox::getUnsandboxedPointer(void* p)
{
	#if defined(_M_X64) || defined(__x86_64__)

		uintptr_t pVal = ((uintptr_t) p);
		if(pVal < wasm_memory_size)
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

void* WasmSandbox::getSandboxedPointer(void* p)
{
	#if defined(_M_X64) || defined(__x86_64__)

		//Wasm memory should be fixed at 4GB
		uintptr_t pVal = ((uintptr_t) p);
		uintptr_t wasm_memoryVal = ((uintptr_t)wasm_memory);
		if(pVal > wasm_memoryVal && (pVal - wasm_memoryVal < wasm_memory_size))
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