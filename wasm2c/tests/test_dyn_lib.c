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

char* simpleEchoTest(char * str)
{
	return str;
}