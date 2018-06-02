#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

void* lib;

void loadLibrary()
{
	lib = dlopen("./libwasm_test_dyn_lib.so", RTLD_NOW);
	if(!lib)
	{
		printf("Dlopen failed: %s\n", dlerror());
		exit(1);
	}

	using voidvoidPtr = void(*)();
	voidvoidPtr wasm_init_module = (voidvoidPtr)(uintptr_t)dlsym(lib, "wasm_init_module");
	if(!wasm_init_module)
	{
		printf("Dlsym wasm_init_module failed: %s\n", dlerror());
		exit(1);
	}
	wasm_init_module();
}

uintptr_t getFunctionPtr(const char* name)
{
	void** symbolAddr = (void**)(dlsym(lib, "Z__simpleAddNoPrintTestZ_iii"));

	if(!symbolAddr || !(*symbolAddr))
	{
		printf("Dlsym %s failed: %s\n", name, dlerror());
		exit(1);
	}

	uintptr_t ret = (uintptr_t) *symbolAddr;
	return ret;
}

int main(int argc, char** argv) {

    printf("Running test\n");
	loadLibrary();

	using ulululPtr = unsigned long(*)(unsigned long, unsigned long);
	ulululPtr simpleAddNoPrintTest = (ulululPtr) getFunctionPtr("simpleAddNoPrintTest");
	auto result = simpleAddNoPrintTest(5, 7);

	if(result != 12)
	{
		printf("Factorial returned the wrong value\n");
		exit(1);
	}

	return 0;
}