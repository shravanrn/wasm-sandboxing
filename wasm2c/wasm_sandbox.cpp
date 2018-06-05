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

    using intvoidPtr = int(*)();
    getSymbol(ret->wasm_register_trap_setjmp, intvoidPtr, wasm_register_trap_setjmp);

	wasm_init_module();

    wasm_rt_memory_t* wasm_memory_st;
    getSymbol(wasm_memory_st, wasm_rt_memory_t*, Z_envZ_memory);

    ret->wasm_memory = wasm_memory_st->data;
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