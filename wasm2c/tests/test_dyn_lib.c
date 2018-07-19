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

void* simpleNullTest(void* shouldBeNull)
{
	if(shouldBeNull == 0)
	{
		return 0;
	}

	return (void*) 1;
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

void* returnNullTest()
{
	return 0;
}

unsigned long getOffset()
{
	struct testStruct2 test;
	uintptr_t offset = (uintptr_t) &(test.ptr_list[1]);
	unsigned long result = offset - ((uintptr_t)&test);
	return result;
}

unsigned long getStructVal(struct testStruct4* p)
{
	return (unsigned long) p->ptr;
}

unsigned char jpeg_nbits_table[] = {
   0,  1,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
   5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
   8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8
};

unsigned char * returnAddr()
{
  return jpeg_nbits_table;
}


int returnElement(int temp)
{
  int nbits = jpeg_nbits_table[temp];
  return nbits;
}


void mallocTest()
{
    // printf("Malloc 1\n");
	// void* p_13_16 = malloc(5151);
	// printf("Malloc 2\n");
	// void* p_14_16 = malloc(5095);
	// printf("Malloc 3\n");
	// void* p_16_16 = malloc(5095);
	// printf("Malloc 5\n");
	// void* p_18_ = malloc(62219);
	void* m_5388000 = malloc(632);
	((char*)m_5388000)[520 + 4*sizeof(void*)] = (uintptr_t)2;
	void* m_5420560 = malloc(991);
	void* m_5421568 = malloc(409920);
	free(m_5420560);
	free(m_5388000);
	void* m49_5388000 = malloc(520);
	void* functionmca_failed = malloc(168);
	printf("Malloc Done\n");
}

void* mallocWrap(size_t size)
{
	return malloc(size);
}