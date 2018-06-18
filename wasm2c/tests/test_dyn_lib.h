#include <stdlib.h>
#include <stdint.h>

typedef int (*CallbackType)(unsigned, const char*, unsigned[1]);

struct testStruct
{
	unsigned long fieldLong;
	const char* fieldString;
	unsigned int fieldBool; 
	char fieldFixedArr[8];
	int (*fieldFnPtr)(unsigned, const char*, unsigned[1]);
};

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b);
int simpleAddTest(int a, int b);
size_t simpleStrLenTest(const char* str);
int simpleCallbackTest(unsigned a, const char* b, CallbackType callback);
char* simpleEchoTest(char * str);
double simpleDoubleAddTest(const double a, const double b);
unsigned long simpleLongAddTest(unsigned long a, unsigned long b);
struct testStruct simpleTestStructVal();

uint32_t test32(uint32_t a);
uint64_t test64(uint64_t a);
float testFloat(float a);
double testDouble(double a);
size_t testLongSize();
size_t testPtrSize();