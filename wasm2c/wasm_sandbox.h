#include <type_traits>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include "wasm-rt-impl.h"

//https://stackoverflow.com/questions/6512019/can-we-get-the-type-of-a-lambda-argument
template<typename Ret, typename... Rest>
Ret return_argument_helper(Ret(*) (Rest...));

template<typename Ret, typename F, typename... Rest>
Ret return_argument_helper(Ret(F::*) (Rest...));

template<typename Ret, typename F, typename... Rest>
Ret return_argument_helper(Ret(F::*) (Rest...) const);

template <typename F>
decltype(return_argument_helper(&F::operator())) return_argument_helper(F);

template <typename T>
using return_argument = decltype(return_argument_helper(std::declval<T>()));


class WasmSandbox
{
private:
	void* lib;
	jmp_buf* (*wasm_get_setjmp_buff)();
	uint32_t(*wasm_malloc)(size_t);
	void(*wasm_free)(uint32_t);
	uint32_t (*wasm_rt_register_func_type)(uint32_t, uint32_t, ...);
	void* wasm_memory;
	size_t wasm_memory_size;

	template<typename T>
	T convertParam(T arg)
	{
		return arg;
	}

	template<typename T>
	T* convertParam(T* arg)
	{
		auto ret = (T*) getSandboxedPointer(arg);
		return ret;
	}

	template<typename T>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_I32;
	}

public:
	static WasmSandbox* createSandbox(const char* path);
	void* symbolLookup(const char* name);

	template<typename T, typename... TArgs>
	return_argument<T*> invokeFunction(T* fnPtr, TArgs... params)
	{
		jmp_buf& buff = *wasm_get_setjmp_buff();
		int trapCode = setjmp(buff);

		if(!trapCode)
		{
			return (*fnPtr)(convertParam(params)...);
		}
		else
		{
			//Function called failed
			printf("Wasm function call failed with trap code: %d\n", trapCode);
			exit(1);
		}
	}

	template<typename TRet, typename... TArgs>
	void registerCallback(TRet(*callback)(TArgs...))
	{
		wasm_rt_elem_t* callbackObj = (wasm_rt_elem_t*) malloc(sizeof(wasm_rt_elem_t));
		uint32_t paramCount = sizeof...(TArgs);
		uint32_t returnCount = 1;
		callbackObj->func_type = wasm_rt_register_func_type(paramCount, returnCount, getWasmType<TArgs>()..., getWasmType<TRet>());
		callbackObj->func = (wasm_rt_anyfunc_t)(void*)callback;
	}

	void* getUnsandboxedPointer(void* p);
	void* getSandboxedPointer(void* p);

	void* mallocInSandbox(size_t size);
	void freeInSandbox(void* ptr);
};