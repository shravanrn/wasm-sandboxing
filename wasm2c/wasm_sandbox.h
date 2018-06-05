#include <type_traits>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    int(*wasm_register_trap_setjmp)();
    uint32_t(*wasm_malloc)(size_t);
    void(*wasm_free)(uint32_t);
    void* wasm_memory;

public:
    static WasmSandbox* createSandbox(const char* path);
    void* symbolLookup(const char* name);

    template<typename T, typename... TArgs>
    return_argument<T*> invokeFunction(T* fnPtr, TArgs... params)
    {
        int trapCode = wasm_register_trap_setjmp();
        if(!trapCode)
        {
            return (*fnPtr)(params...);
        }
        else
        {
            //Function called failed
            printf("Wasm function call failed with trap code: %d\n", trapCode);
            exit(1);
        }
    }

    void* getUnsandboxedPointer(void* p);
    void* getSandboxedPointer(void* p);

    void* mallocInSandbox(size_t size);
    void freeInSandbox(void* ptr);
};