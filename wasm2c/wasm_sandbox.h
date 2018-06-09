#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <type_traits>
#include <vector>
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

namespace WasmSandboxImpl
{
	template<typename T>
	struct wrapper {};

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	uint32_t convertArg(wrapper<T> arg);

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	uint64_t convertArg(wrapper<T> arg);

	float convertArg(wrapper<float> arg);

	double convertArg(wrapper<double> arg);

	uint32_t convertArg(wrapper<void*> arg);

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
	>::type* = nullptr>
	uint32_t convertArg(wrapper<T> arg);

	void convertArg(wrapper<void> arg);
}

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

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	uint32_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
	{
		return arg;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	uint64_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
	{
		return arg;
	}

	float serializeArg(std::vector<void*>& allocatedPointers, float arg)
	{
		return arg;
	}

	double serializeArg(std::vector<void*>& allocatedPointers, double arg)
	{
		return arg;
	}

	uint32_t serializeArg(std::vector<void*>& allocatedPointers, void* arg)
	{
		uint32_t ret = (uintptr_t) getSandboxedPointer(arg);
		return ret;
	}

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
	>::type* = nullptr>
	uint32_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
	{
		T* argInSandbox = mallocInSandbox(sizeof(T));
		*argInSandbox = arg;
		allocatedPointers.push_back(argInSandbox);
		uint32_t ret = (uintptr_t) getSandboxedPointer(argInSandbox);
		return ret;
	}

	template<typename T>
	using wasm_return_type = decltype(WasmSandboxImpl::convertArg(std::declval<WasmSandboxImpl::wrapper<T>>()));

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	T convertReturnValue(uint32_t arg)
	{
		return arg;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value
	>::type* = nullptr>
	T convertReturnValue(uint64_t arg)
	{
		return arg;
	}

	float convertReturnValue(float arg)
	{
		return arg;
	}

	double convertReturnValue(double arg)
	{
		return arg;
	}

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	T convertReturnValue(uint32_t arg)
	{
		auto ret = (T) getUnsandboxedPointer((void*)(uintptr_t)arg);
		return ret;
	}

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
	>::type* = nullptr>
	T convertReturnValue(uint32_t arg)
	{
		T* ret = (T*) getUnsandboxedPointer((void*)(uintptr_t)arg);
		return *ret;
	}

	template<typename T, typename T2=void>
	wasm_rt_type_t getWasmType();

	template<typename T, typename std::enable_if<
			sizeof(T) <= 4
			&& !std::is_floating_point<T>::value
			&& !std::is_pointer<T>::value
		>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_I32;
	}

	template<typename TRet, typename... TArgs>
	typename std::enable_if<!std::is_void<TRet>::value,
	TRet>::type
	invokeFunctionWithArgs(void* fnPtr, std::vector<void*>& allocatedPointers, TArgs... args)
	{
		using WasmRetType = wasm_return_type<TRet>;
		using TargetFuncType = WasmRetType(*)(TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		auto ret = (*fnPtrCast)(args...);

		for(auto ptr : allocatedPointers)
		{
			freeInSandbox(ptr);
		}

		TRet convertedRet = convertReturnValue<TRet>(ret);
		return ret;
	}

	template<typename TRet, typename... TArgs>
	typename std::enable_if<std::is_void<TRet>::value,
	void>::type
	invokeFunctionWithArgs(void* fnPtr, std::vector<void*>& allocatedPointers, TArgs... args)
	{
		using TargetFuncType = void(*)(TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		(*fnPtrCast)(args...);

		for(auto ptr : allocatedPointers)
		{
			freeInSandbox(ptr);
		}
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
			std::vector<void*> allocatedPointers;
			return invokeFunctionWithArgs<return_argument<T*>>((void*) fnPtr, allocatedPointers, serializeArg(allocatedPointers, params)...);
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