#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <type_traits>
#include <vector>
#include <map>
#include <mutex>

#ifndef WASM_RT_H_
	//Need some structs from the wasm runtime, but don't want a dependency on that header, so include it manually here
	typedef enum {
		WASM_RT_I32,
		WASM_RT_I64,
		WASM_RT_F32,
		WASM_RT_F64,
	} wasm_rt_type_t;

#endif

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

class WasmSandbox;

namespace WasmSandboxImpl
{
	template<typename T>
	struct wrapper {};

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	uint32_t convertArg(wrapper<T> arg);

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	uint64_t convertArg(wrapper<T> arg);

	float convertArg(wrapper<float> arg);

	double convertArg(wrapper<double> arg);

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	uint32_t convertArg(wrapper<T> arg);

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
	>::type* = nullptr>
	uint32_t convertArg(wrapper<T> arg);

	void convertArg(wrapper<void> arg);

	extern thread_local WasmSandbox* CurrThreadSandbox;
}

class WasmSandboxCallback
{
public:
	uint32_t callbackSlot;
	uintptr_t originalCallback;
	uintptr_t callbackStub;
};

class WasmSandbox
{
private:
	void* lib;
	jmp_buf* (*wasm_get_setjmp_buff)();
	uint32_t(*wasm_malloc)(size_t);
	void(*wasm_free)(uint32_t);
	uint32_t (*wasm_rt_register_func_type_with_lists)(std::vector<wasm_rt_type_t> *, std::vector<wasm_rt_type_t> *);
	uint32_t (*wasm_rt_register_func)(void(*func)(), uint32_t funcType);
	void (*wasm_ret_unregister_func)(uint32_t slotNumber);
	uint32_t (*wasm_get_current_indirect_call_num)();
	void* wasm_memory;
	size_t wasm_memory_size;

	std::map<uint32_t, void*> registerCallbackMap;
	std::mutex registeredCallbackLock;

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	uint32_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
	{
		return arg;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
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

	uint32_t serializeArg(std::vector<void*>& allocatedPointers, WasmSandboxCallback arg)
	{
		return arg.callbackSlot;
	}

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	uint32_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
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
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	T convertReturnValue(uint32_t arg)
	{
		return arg;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
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

	template<typename T, typename std::enable_if<
		sizeof(T) <= 4 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_I32;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_I64;
	}

	template<typename T, typename std::enable_if<
		std::is_same<T, float>::value
	>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_F32;
	}

	template<typename T, typename std::enable_if<
		std::is_same<T, double>::value
	>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_F64;
	}

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	wasm_rt_type_t getWasmType()
	{
		return WASM_RT_I32;
	}

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
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
		auto oldCurrThreadSandbox = WasmSandboxImpl::CurrThreadSandbox;
		WasmSandboxImpl::CurrThreadSandbox = this;

		using WasmRetType = wasm_return_type<TRet>;
		using TargetFuncType = WasmRetType(*)(TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		auto ret = (*fnPtrCast)(args...);

		WasmSandboxImpl::CurrThreadSandbox = oldCurrThreadSandbox;

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
		auto oldCurrThreadSandbox = WasmSandboxImpl::CurrThreadSandbox;
		WasmSandboxImpl::CurrThreadSandbox = this;

		using TargetFuncType = void(*)(TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		(*fnPtrCast)(args...);

		WasmSandboxImpl::CurrThreadSandbox = oldCurrThreadSandbox;

		for(auto ptr : allocatedPointers)
		{
			freeInSandbox(ptr);
		}
	}

	WasmSandboxCallback registerCallbackImpl(void(*callback)(), void(*callbackStub)(), std::vector<wasm_rt_type_t> params, std::vector<wasm_rt_type_t> results);

	template <typename... Types>
	struct invokeCallbackTargetHelper {};

	template<class TFunc,
		template<class...> class TSandboxArgsWrapper, 
		template<class...> class TArgsWrapper,
		class... TSandboxArgs,
		class... TArgs,
		class... TExtraArgs
	>
	return_argument<TFunc> invokeCallbackTarget(TFunc targetFunc, 
		TSandboxArgsWrapper<TSandboxArgs...> wrap1,
		TArgsWrapper<TArgs...> wrap2,
		TExtraArgs... args
	)
	{
		return targetFunc(args...);
	}

	template<class TFunc,
		template<class...> class TSandboxArgsWrapper,
		template<class...> class TArgsWrapper,
		class TSandboxArg, class... TSandboxArgs,
		class TArg, class... TArgs,
		class... TExtraArgs
	>
	return_argument<TFunc> invokeCallbackTarget(TFunc targetFunc, 
		TSandboxArgsWrapper<TSandboxArg, TSandboxArgs...> wrap1,
		TArgsWrapper<TArg, TArgs...> wrap2,
		TSandboxArg sandboxArg,
		TExtraArgs... args
	)
	{
		auto convertedValue = convertReturnValue<TArg>(sandboxArg);
		return invokeCallbackTarget(targetFunc, 
			invokeCallbackTargetHelper<TSandboxArgs...>(),
			invokeCallbackTargetHelper<TArgs...>(),
			args..., convertedValue
		);
	}

	template<typename TRet, typename... TArgs>
	wasm_return_type<TRet> callbackStubImpl(wasm_return_type<TArgs>... args)
	{
		std::vector<void*> allocatedPointers;
		using TargetFuncType = TRet(*)(TArgs...);
		TargetFuncType convFuncPtr;
		uint32_t callbackSlot = wasm_get_current_indirect_call_num();

		{
			std::lock_guard<std::mutex> lockGuard (registeredCallbackLock);
			convFuncPtr = (TargetFuncType) registerCallbackMap[callbackSlot];
		}

		auto ret = invokeCallbackTarget(convFuncPtr, 
			invokeCallbackTargetHelper<wasm_return_type<TArgs>...>(),
			invokeCallbackTargetHelper<TArgs...>(),
			args...
		);
		TRet retConv = serializeArg(allocatedPointers, ret);
		return retConv;
	}

	template<typename TRet, typename... TArgs>
	static wasm_return_type<TRet> callbackStub(wasm_return_type<TArgs>... args)
	{
		return WasmSandboxImpl::CurrThreadSandbox->callbackStubImpl<TRet, TArgs...>(args...);
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
	WasmSandboxCallback registerCallback(TRet(*callback)(TArgs...))
	{
		std::vector<wasm_rt_type_t> params { getWasmType<TArgs>()...};
		std::vector<wasm_rt_type_t> returns { getWasmType<TRet>()};
		using voidVoidType = void(*)();

		auto callbackStubRef = callbackStub<TRet, TArgs...>; 
		auto ret = registerCallbackImpl((voidVoidType)(void*)callback, (voidVoidType)(void*)callbackStubRef, params, returns);
		return ret;
	}

	void unregisterCallback(WasmSandboxCallback callback);

	void* getUnsandboxedPointer(void* p);
	void* getSandboxedPointer(void* p);

	void* mallocInSandbox(size_t size);
	void freeInSandbox(void* ptr);
};