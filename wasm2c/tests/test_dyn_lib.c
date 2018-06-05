#include "test_dyn_lib.h"

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b)
{
	return a + b;
}

const char* strAppend()
{
    return "Hello World!";
}

char* simpleEchoTest(char * str)
{
	return str;
}