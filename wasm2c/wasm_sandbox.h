#ifndef WASM_SANDBOX_H_
#define WASM_SANDBOX_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <type_traits>
#include <vector>
#include <map>
#include <mutex>
#include <string>

//Need some structs from the wasm runtime, but don't want a dependency on that header, so include it with a new name here
typedef enum {
	DUP_WASM_RT_I32,
	DUP_WASM_RT_I64,
	DUP_WASM_RT_F32,
	DUP_WASM_RT_F64,
} wasm_rt_type_t_dup;

class WasmSandbox;

namespace WasmSandboxImpl
{
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
	std::string ExportPrefix;
	jmp_buf* (*wasm_get_setjmp_buff)();
	uint32_t(*wasm_malloc)(size_t);
	void(*wasm_free)(uint32_t);
	uint32_t (*wasm_rt_register_func_type_with_lists)(std::vector<wasm_rt_type_t_dup> *, std::vector<wasm_rt_type_t_dup> *);
	uint32_t (*wasm_rt_register_func)(void(*func)(), uint32_t funcType);
	void (*wasm_ret_unregister_func)(uint32_t slotNumber);
	void* (*wasm_rt_get_registered_func)(uint32_t slotNumber);
	uint32_t (*wasm_get_current_indirect_call_num)();
	void (*checkStackCookie)();
	uint32_t (*wasm_get_heap_base)();
	void* wasm_memory;
	size_t wasm_memory_size;

	std::map<uint32_t, void*> registerCallbackMap;
	std::map<uint32_t, void*> callbackStateMap;
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

	uint32_t serializeArg(std::vector<void*>& allocatedPointers, WasmSandboxCallback* arg)
	{
		return arg->callbackSlot;
	}

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	uint32_t serializeArg(std::vector<void*>& allocatedPointers, T arg)
	{
		uint32_t ret = (uintptr_t) (const void*) arg;
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

	template<typename T,typename std::enable_if<std::is_same<T, float>::value>::type* = nullptr>
	float convertReturnValue(float arg)
	{
		return arg;
	}

	template<typename T,typename std::enable_if<std::is_same<T, double>::value>::type* = nullptr>
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
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_I32;
	}

	template<typename T, typename std::enable_if<
		!(sizeof(T) <= 4) && sizeof(T) <= 8 && std::is_fundamental<T>::value && !std::is_floating_point<T>::value && !std::is_pointer<T>::value
	>::type* = nullptr>
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_I64;
	}

	template<typename T, typename std::enable_if<
		std::is_same<T, float>::value
	>::type* = nullptr>
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_F32;
	}

	template<typename T, typename std::enable_if<
		std::is_same<T, double>::value
	>::type* = nullptr>
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_F64;
	}

	template<typename T, typename std::enable_if<
		std::is_pointer<T>::value
	>::type* = nullptr>
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_I32;
	}

	template<typename T, typename std::enable_if<
		std::is_class<T>::value
	>::type* = nullptr>
	wasm_rt_type_t_dup getWasmType()
	{
		return DUP_WASM_RT_I32;
	}

	//Handle the fact that struct returns are passed as out params per the calling convention
	template<typename TRet, typename... TArgs>
	typename std::enable_if<!std::is_class<TRet>::value,
	TRet>::type invokeFunctionWithArgsAndRetParam(void* fnPtr, TArgs... args)
	{
		using WasmRetType = wasm_return_type<TRet>;
		using TargetFuncType = WasmRetType(*)(TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		auto ret = (*fnPtrCast)(args...);

		TRet convertedRet = convertReturnValue<TRet>(ret);
		return convertedRet;
	}

	template<typename TRet, typename... TArgs>
	typename std::enable_if<std::is_class<TRet>::value,
	TRet>::type invokeFunctionWithArgsAndRetParam(void* fnPtr, TArgs... args)
	{
		TRet* ret = (TRet*) mallocInSandbox(sizeof(TRet));
		uint32_t retp = (uintptr_t) getSandboxedPointer(ret);

		using TargetFuncType = void(*)(uint32_t, TArgs...);
		TargetFuncType fnPtrCast = (TargetFuncType) fnPtr;
		(*fnPtrCast)(retp, args...);

		TRet retCopy = *ret;
		freeInSandbox(ret);
		return retCopy;
	}

	template<typename TRet, typename... TArgs>
	typename std::enable_if<!std::is_void<TRet>::value,
	TRet>::type
	invokeFunctionWithArgs(void* fnPtr, std::vector<void*>& allocatedPointers, TArgs... args)
	{
		auto oldCurrThreadSandbox = WasmSandboxImpl::CurrThreadSandbox;
		WasmSandboxImpl::CurrThreadSandbox = this;

		auto ret = invokeFunctionWithArgsAndRetParam<TRet, TArgs...>(fnPtr, args...);

		WasmSandboxImpl::CurrThreadSandbox = oldCurrThreadSandbox;

		checkStackCookie();

		for(auto ptr : allocatedPointers)
		{
			freeInSandbox(ptr);
		}

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

		checkStackCookie();

		for(auto ptr : allocatedPointers)
		{
			freeInSandbox(ptr);
		}
	}

	WasmSandboxCallback* registerCallbackImpl(void* state, void(*callback)(), void(*callbackStub)(), std::vector<wasm_rt_type_t_dup> params, std::vector<wasm_rt_type_t_dup> results);

	template <typename... Types>
	struct invokeCallbackTargetHelper {};

	template<class TFunc,
		template<class...> class TSandboxArgsWrapper, 
		template<class...> class TArgsWrapper,
		class... TSandboxArgs,
		class... TArgs,
		class... TExtraArgs
	>
	WasmSandboxImpl::return_argument<TFunc> invokeCallbackTarget(TFunc targetFunc, 
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
	WasmSandboxImpl::return_argument<TFunc> invokeCallbackTarget(TFunc targetFunc, 
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
	typename std::enable_if<!std::is_void<TRet>::value,
	wasm_return_type<TRet>>::type callbackStubImpl(wasm_return_type<TArgs>... args)
	{
		std::vector<void*> allocatedPointers;
		using TargetFuncType = TRet(*)(void*, TArgs...);
		TargetFuncType convFuncPtr;
		uint32_t callbackSlot = wasm_get_current_indirect_call_num();
		void* state;

		{
			std::lock_guard<std::mutex> lockGuard (registeredCallbackLock);
			convFuncPtr = (TargetFuncType) registerCallbackMap[callbackSlot];
			state = callbackStateMap[callbackSlot];
		}

		auto ret = invokeCallbackTarget(convFuncPtr, 
			invokeCallbackTargetHelper<wasm_return_type<TArgs>...>(),
			invokeCallbackTargetHelper<TArgs...>(),
			args...,
			state
		);
		TRet retConv = serializeArg(allocatedPointers, ret);
		return retConv;
	}

	template<typename TRet, typename... TArgs>
	typename std::enable_if<std::is_void<TRet>::value,
	wasm_return_type<TRet>>::type callbackStubImpl(wasm_return_type<TArgs>... args)
	{
		std::vector<void*> allocatedPointers;
		using TargetFuncType = TRet(*)(void*, TArgs...);
		TargetFuncType convFuncPtr;
		uint32_t callbackSlot = wasm_get_current_indirect_call_num();
		void* state;

		{
			std::lock_guard<std::mutex> lockGuard (registeredCallbackLock);
			convFuncPtr = (TargetFuncType) registerCallbackMap[callbackSlot];
			state = callbackStateMap[callbackSlot];
		}

		invokeCallbackTarget(convFuncPtr, 
			invokeCallbackTargetHelper<wasm_return_type<TArgs>...>(),
			invokeCallbackTargetHelper<TArgs...>(),
			args...,
			state
		);
	}

	template<typename TRet, typename... TArgs>
	static wasm_return_type<TRet> callbackStub(wasm_return_type<TArgs>... args)
	{
		return WasmSandboxImpl::CurrThreadSandbox->callbackStubImpl<TRet, TArgs...>(args...);
	}

public:
	static WasmSandbox* createSandbox(const char* path);
	void* symbolLookup(const char* name);

	template<typename TRet, typename... TFuncArgs, typename... TArgs>
	TRet invokeFunction(TRet(*fnPtr)(TFuncArgs...), TArgs... params)
	{
		jmp_buf& buff = *wasm_get_setjmp_buff();
		int trapCode = setjmp(buff);

		if(!trapCode)
		{
			std::vector<void*> allocatedPointers;
			return invokeFunctionWithArgs<TRet>((void*) fnPtr, allocatedPointers, serializeArg(allocatedPointers, (TFuncArgs)params)...);
		}
		else
		{
			//Function called failed
			printf("Wasm function call failed with trap code: %d\n", trapCode);
			abort();
		}
	}

	template<typename TRet, typename std::enable_if<std::is_void<TRet>::value>::type* = nullptr>
	std::vector<wasm_rt_type_t_dup> getCallbackReturnWasmVec()
	{
		std::vector<wasm_rt_type_t_dup> ret;
		return ret;
	}

	template<typename TRet, typename std::enable_if<!std::is_void<TRet>::value>::type* = nullptr>
	std::vector<wasm_rt_type_t_dup> getCallbackReturnWasmVec()
	{
		std::vector<wasm_rt_type_t_dup> ret{ getWasmType<TRet>()};
		return ret;
	}

	template<typename TRet, typename... TArgs>
	WasmSandboxCallback* registerCallback(TRet(*callback)(void*, TArgs...), void* state)
	{
		std::vector<wasm_rt_type_t_dup> params { getWasmType<TArgs>()...};
		std::vector<wasm_rt_type_t_dup> returns = getCallbackReturnWasmVec<TRet>();
		using voidVoidType = void(*)();

		auto callbackStubRef = callbackStub<TRet, TArgs...>; 
		auto ret = registerCallbackImpl(state, (voidVoidType)(void*)callback, (voidVoidType)(void*)callbackStubRef, params, returns);
		return ret;
	}

	void unregisterCallback(WasmSandboxCallback* callback);
	void* getUnsandboxedFuncPointer(const void* p);

	void* getUnsandboxedPointer(const void* p);
	void* getSandboxedPointer(const void* p);

	int isAddressInNonSandboxMemoryOrNull(const void* p);
	int isAddressInSandboxMemoryOrNull(const void* p);

	void* mallocInSandbox(size_t size);
	void freeInSandbox(void* ptr);

	size_t getTotalMemory();
	
	inline void* getSandboxMemoryBase()
	{
		return wasm_memory;
	}
};

#endif