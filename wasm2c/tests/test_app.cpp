#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include "../wasm_sandbox.h"

int invokeSimpleAddTest(WasmSandbox* sandbox, int a, int b)
{
	using fnType = unsigned long(*)(unsigned long, unsigned long);
	fnType fn = (fnType) sandbox->symbolLookup("simpleAddNoPrintTest");

	auto result = sandbox->invokeFunction(fn, a, b);
	return result;
}

size_t invokeSimpleStrLenTestWithHeapString(WasmSandbox* sandbox, const char* str)
{
	using fnType = size_t (*)(const char*);
	fnType fn = (fnType) sandbox->symbolLookup("simpleStrLenTest");

	char* strInSandbox = (char*) sandbox->mallocInSandbox(strlen(str) + 1);
	strcpy(strInSandbox, str);

	auto result = sandbox->invokeFunction(fn, strInSandbox);
	sandbox->freeInSandbox(strInSandbox);
	return result;
}

int main(int argc, char** argv) {

	printf("Running test\n");
	WasmSandbox* sandbox = WasmSandbox::createSandbox("./libwasm_test_dyn_lib.so");

	if(!sandbox)
	{
		printf("Sandbox creation failed\n");
		exit(1);
	}

	if(invokeSimpleAddTest(sandbox, 2, 3) != 5)
	{
		printf("Test 1: Failed\n");
		exit(1);
	}

	// if(invokeSimpleStrLenTestWithStackString(sandbox, testParams->simpleStrLenTestResult, "Hello") != 5)
	// {
	// 	printf("Test 2: Failed\n");
	// 	exit(1);
	// }

	if(invokeSimpleStrLenTestWithHeapString(sandbox, "Hello") != 5)
	{
		printf("Test 3: Failed\n");
		exit(1);
	}

	printf("Tests Completed\n");

	return 0;
}