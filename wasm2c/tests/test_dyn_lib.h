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

struct testStruct2
{
	void* ptr_list[2];
};

struct testStruct3
{
	uint32_t val;
	void* ptr;
};

struct testStruct4
{
	void* ptr_list[2];
	void* ptr;
};

unsigned long simpleAddNoPrintTest(unsigned long a, unsigned long b);
int simpleAddTest(int a, int b);
size_t simpleStrLenTest(const char* str);
int simpleCallbackTest(unsigned a, const char* b, CallbackType callback);
char* simpleEchoTest(char * str);
double simpleDoubleAddTest(const double a, const double b);
unsigned long simpleLongAddTest(unsigned long a, unsigned long b);
struct testStruct simpleTestStructVal();
void* simpleNullTest(void* shouldBeNull);
unsigned long getOffset();
unsigned long getStructVal(struct testStruct4* p);
int returnElement(int temp);