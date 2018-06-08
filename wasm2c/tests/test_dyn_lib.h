#include <stdlib.h>
#include <stdint.h>

typedef int (*CallbackType)(unsigned, const char*, unsigned[1]);

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b);
size_t simpleStrLenTest(const char* str);
int simpleCallbackTest(unsigned a, const char* b, CallbackType callback);
char* simpleEchoTest(char * str);