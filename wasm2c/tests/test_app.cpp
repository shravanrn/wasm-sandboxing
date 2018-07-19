#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <limits.h>
#include "../wasm_sandbox.h"
#include "test_dyn_lib.h"

int invokeSimpleAddTest(WasmSandbox* sandbox, int a, int b)
{
	using fnType = int (*)(int, int);
	fnType fn = (fnType) sandbox->symbolLookup("simpleAddTest");

	auto result = sandbox->invokeFunction(fn, a, b);
	return result;
}

//////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////

int invokeSimpleCallbackTest_callback(unsigned a, const char* b, unsigned c[1])
{
	return a + strlen(b);
}

int invokeSimpleCallbackTest(WasmSandbox* sandbox, unsigned a, const char* b, int (*callback)(unsigned, const char*, unsigned[1]))
{
	using fnType = int (*)(unsigned, const char*, CallbackType);
	fnType fn = (fnType) sandbox->symbolLookup("simpleCallbackTest");

	char* bInSandbox = (char*) sandbox->mallocInSandbox(strlen(b) + 1);
	strcpy(bInSandbox, b);
	auto registeredCallback = sandbox->registerCallback(callback);

	auto result = sandbox->invokeFunction(fn, a, bInSandbox, registeredCallback);
	sandbox->freeInSandbox(bInSandbox);
	sandbox->unregisterCallback(registeredCallback);
	return result;
}

//////////////////////////////////////////////////////////////////

int invokeSimpleEchoTestPassed(WasmSandbox* sandbox, const char* str)
{
	char* strInSandbox;
	char* retStr;
	int ret;

	//str is allocated in our heap, not the sandbox's heap
	if(!sandbox->isAddressInNonSandboxMemoryOrNull(str))
	{
		return 0;
	}

	strInSandbox = (char*) sandbox->mallocInSandbox(strlen(str) + 1);
	strcpy(strInSandbox, str);

	//str is allocated in sandbox heap, not our heap
	if(!sandbox->isAddressInSandboxMemoryOrNull(strInSandbox))
	{
		return 0;
	}

	using fnType = char* (*)(char *);
	fnType fn = (fnType) sandbox->symbolLookup("simpleEchoTest");

	retStr = sandbox->invokeFunction(fn, strInSandbox);

	//retStr is allocated in sandbox heap, not our heap
	if(!sandbox->isAddressInSandboxMemoryOrNull(retStr))
	{
		return 0;
	}

	ret = strcmp(str, retStr) == 0;

	sandbox->freeInSandbox(strInSandbox);

	return ret;
}

//////////////////////////////////////////////////////////////////

double invokeSimpleDoubleAddTest(WasmSandbox* sandbox, double a, double b)
{
	using fnType = double (*)(double, double);
	fnType fn = (fnType) sandbox->symbolLookup("simpleDoubleAddTest");

	auto result = sandbox->invokeFunction(fn, a, b);
	return result;
}

//////////////////////////////////////////////////////////////////

unsigned long invokeSimpleLongAddTest(WasmSandbox* sandbox, unsigned long a, unsigned long b)
{
	using fnType = unsigned long (*)(unsigned long, unsigned long);
	fnType fn = (fnType) sandbox->symbolLookup("simpleLongAddTest");

	auto result = sandbox->invokeFunction(fn, a, b);
	return result;
}

//////////////////////////////////////////////////////////////////

int invokeSimpleTestStructValTest(WasmSandbox* sandbox)
{
	using fnType = struct testStruct (*)();
	fnType fn = (fnType) sandbox->symbolLookup("simpleTestStructVal");

	auto result = sandbox->invokeFunction(fn);

	if(result.fieldLong != 7 || 
		strcmp((char*) sandbox->getUnsandboxedPointer(result.fieldString), "Hello") != 0 || 
		result.fieldBool != 1 || 
		strcmp(result.fieldFixedArr, "Bye"))
	{
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////

int invokeSimpleNullTest(WasmSandbox* sandbox)
{
	using fnType = void* (*)(void*);
	fnType fn = (fnType) sandbox->symbolLookup("simpleNullTest");

	auto result = sandbox->invokeFunction(fn, (void*) 0);

	if(result == nullptr)
	{
		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////

unsigned long invokeSimpleOffsetTest(WasmSandbox* sandbox)
{
	using fnType = unsigned long (*)();
	fnType fn = (fnType) sandbox->symbolLookup("getOffset");

	auto result = sandbox->invokeFunction(fn);
	struct testStruct2 test;
	uintptr_t offset = (uintptr_t) &(test.ptr_list[1]);
	auto expectedResult = offset - ((uintptr_t)&test);
	
	if(result != expectedResult)
	{
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////

unsigned long invokeSimpleStructValTest(WasmSandbox* sandbox)
{
	using fnType = unsigned long (*)(struct testStruct4*);
	fnType fn = (fnType) sandbox->symbolLookup("getStructVal");

	struct testStruct4* test = (struct testStruct4*) sandbox->mallocInSandbox(sizeof(struct testStruct4));
	test->ptr = (void*) 0x1234;

	auto result = sandbox->invokeFunction(fn, test);
	
	if(result != 0x1234)
	{
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////

unsigned long invokeSimpleReturnElementTest(WasmSandbox* sandbox)
{
	using fnType = int (*)(int);
	fnType fn = (fnType) sandbox->symbolLookup("returnElement");

	auto result = sandbox->invokeFunction(fn, 1);
	
	if(result != 1)
	{
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////

void invokeSimpleMallocTest(WasmSandbox* sandbox,  void (*fn)(void))
{
	sandbox->invokeFunction(fn);
}

//////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

	printf("Running test\n");
	WasmSandbox* sandbox = WasmSandbox::createSandbox("./libwasm_test_dyn_lib.so");

	if(!sandbox)
	{
		printf("Sandbox creation failed\n");
		exit(1);
	}

	// printf("Loop?: ");
	// putchar(' ');
	// int loop = 0;
	// scanf("%d", &loop);
	// if(loop)
	// {
	// 	printf("Malloc 1\n");
	// 	auto p_13_16 = sandbox->mallocInSandbox(5151);
	// 	printf("Malloc 2\n");
	// 	auto p_14_16 = sandbox->mallocInSandbox(5095);
	// 	// auto p_15_5184 = sandbox->mallocInSandbox(2591);
	// 	printf("Malloc 3\n");
	// 	auto p_16_16 = sandbox->mallocInSandbox(5095);
	// 	// printf("Malloc 4\n");
	// 	// auto p_17_ = sandbox->mallocInSandbox(2591);
	// 	printf("Malloc 5\n");
	// 	auto p_18_ = sandbox->mallocInSandbox(62219);
	// 	printf("Malloc Done\n");
	// 	// exit(0);
	// }

	using fnType = void (*)(void);
	fnType fn = (fnType) sandbox->symbolLookup("mallocTest");
	invokeSimpleMallocTest(sandbox, fn);
	// exit(0);

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

	if(invokeSimpleCallbackTest(sandbox, 4, "Hello", invokeSimpleCallbackTest_callback) != 10)
	{
		printf("Test 4: Failed\n");
		exit(1);
	}

	// if(!fileTestPassed(sandbox))
	// {
	// 	printf("Test 5: Failed\n");
	// 	exit(1);	
	// }

	if(!invokeSimpleEchoTestPassed(sandbox, "Hello"))
	{
		printf("Test 6: Failed\n");
		exit(1);	
	}

	if(invokeSimpleDoubleAddTest(sandbox, 1.0, 2.0) != 3.0)
	{
		printf("Test 7: Failed\n");
		exit(1);
	}

	if(invokeSimpleLongAddTest(sandbox, ULONG_MAX - 1, 1) != ULONG_MAX)
	{
		printf("Test 8: Failed\n");
		exit(1);	
	}

	if(!invokeSimpleTestStructValTest(sandbox))
	{
		printf("Test 9: Failed\n");
		exit(1);	
	}

	if(!invokeSimpleNullTest(sandbox))
	{
		printf("Test 10: Failed\n");
		exit(1);	
	}

	if(!invokeSimpleOffsetTest(sandbox))
	{
		printf("Test 11: Failed\n");
		exit(1);	
	}

	if(!invokeSimpleStructValTest(sandbox))
	{
		printf("Test 12: Failed\n");
		exit(1);	
	}

	if(!invokeSimpleReturnElementTest(sandbox))
	{
		printf("Test 13: Failed\n");
		exit(1);	
	}

	printf("Tests Completed\n");

	return 0;
}