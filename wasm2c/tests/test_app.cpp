#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include "../wasm_sandbox.h"

int main(int argc, char** argv) {

    printf("Running test\n");
    WasmSandbox* sandbox = WasmSandbox::createSandbox("./libwasm_test_dyn_lib.so");

	using ulululPtr = unsigned long(*)(unsigned long, unsigned long);
	ulululPtr simpleAddNoPrintTest = (ulululPtr) sandbox->symbolLookup("simpleAddNoPrintTest");

    auto result = sandbox->invokeFunction(simpleAddNoPrintTest, 5, 7);

	if(result != 12)
	{
		printf("Factorial returned the wrong value\n");
		exit(1);
	}

	return 0;
}