#include "test_dyn_lib.h"
#include <string.h>

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b)
{
	return a + b;
}

size_t simpleStrLenTest(const char* str)
{
	return strlen(str);
}

int simpleCallbackTest(unsigned a, const char* b, CallbackType callback)
{
	int ret = callback(a + 1, b, &a);
	return ret;
}

char* simpleEchoTest(char * str)
{
	return str;
}

unsigned long getVal()
{
	return sizeof(void*);
}