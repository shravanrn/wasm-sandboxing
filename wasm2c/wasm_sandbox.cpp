#include "wasm_sandbox.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string>

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
	voidvoidPtr wasm_init_module = (voidvoidPtr)(uintptr_t)dlsym(ret->lib, "wasm_init_module");
	if(!wasm_init_module)
	{
		printf("WasmSandbox Dlsym wasm_init_module failed: %s\n", dlerror());
		return nullptr;
	}

	wasm_init_module();
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