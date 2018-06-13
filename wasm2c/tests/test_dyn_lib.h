#include <stdlib.h>
#include <stdint.h>

typedef int (*CallbackType)(unsigned, const char*, unsigned[1]);

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b);
size_t simpleStrLenTest(const char* str);
int simpleCallbackTest(unsigned a, const char* b, CallbackType callback);
char* simpleEchoTest(char * str);

uint32_t test32(uint32_t a);
uint64_t test64(uint64_t a);
float testFloat(float a);
double testDouble(double a);
size_t testLongSize();
size_t testPtrSize();