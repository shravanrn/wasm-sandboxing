#include "test_dyn_lib.h"
#include <string.h>
#include <stdio.h>

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b)
{
	return a + b;
}

int simpleAddTest(int a, int b)
{
	printf("simpleAddTest\n");
	fflush(stdout);
	return a + b;
}

size_t simpleStrLenTest(const char* str)
{
	printf("simpleStrLenTest\n");
	return strlen(str);
}

int simpleCallbackTest(unsigned a, const char* b, CallbackType callback)
{
	printf("simpleCallbackTest\n");
	int ret = callback(a + 1, b, &a);
	return ret;
}

char* simpleEchoTest(char * str)
{
	printf("simpleEchoTest\n");
	return str;
}

double simpleDoubleAddTest(const double a, const double b)
{
	printf("simpleDoubleAddTest\n");
	return a + b;
}

unsigned long simpleLongAddTest(unsigned long a, unsigned long b)
{
	printf("simpleLongAddTest\n");
	fflush(stdout);
	return a + b;
}

struct testStruct simpleTestStructVal()
{
	printf("simpleTestStructVal\n");
	struct testStruct ret;
	ret.fieldLong = 7;
	ret.fieldString = "Hello";
	//explicitly mess up the top bits of the pointer. The sandbox checks outside the sandbox should catch this
	ret.fieldString = (char *)((((uintptr_t) ret.fieldString) & 0xFFFFFFFF) | 0x1234567800000000);
	ret.fieldBool = 1;
	strcpy(ret.fieldFixedArr, "Bye");
	return ret;
}

/////////////////////////////////

unsigned long getVal()
{
	return sizeof(void*);
}

uint32_t test32(uint32_t a)
{
	return a;
}
uint64_t test64(uint64_t a)
{
	return a >> 32;
}
float testFloat(float a)
{
	return a;
}
double testDouble(double a)
{
	return a;
}

size_t testLongSize()
{
	return sizeof(long);
}

size_t testPtrSize()
{
	return sizeof(void*);
}

int printsize_tSize()
{
	return sizeof(size_t);
}
int printchar_pSize()
{
	return sizeof(char*);
}
int printintSize()
{
	return sizeof(int);
}
void printTest()
{
	printf("Hello!");
}