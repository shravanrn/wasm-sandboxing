#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "wasm-rt.h"

//Create 2 symbols formats for runtime - the emscripten(fastcomp) backend as well as the native llvm wasm backend
//Syscalls WASM32 with LP64 model in C
uint64_t (*Z_envZ___syscall140Z_jii)(uint32_t, uint32_t);
uint64_t (*Z_envZ____syscall140Z_jii)(uint32_t, uint32_t);

uint64_t (*Z_envZ___syscall146Z_jii)(uint32_t, uint32_t);
uint64_t (*Z_envZ____syscall146Z_jii)(uint32_t, uint32_t);

uint64_t (*Z_envZ___syscall54Z_jii)(uint32_t, uint32_t);
uint64_t (*Z_envZ____syscall54Z_jii)(uint32_t, uint32_t);

uint64_t (*Z_envZ___syscall6Z_jii)(uint32_t, uint32_t);
uint64_t (*Z_envZ____syscall6Z_jii)(uint32_t, uint32_t);

extern wasm_rt_memory_t *Z_envZ_memory;
extern uint32_t *Z_envZ_memoryBaseZ_i;

uint64_t sys_writev(uint32_t syscallnum, uint32_t args);
uint64_t sys_ioctl(uint32_t syscallnum, uint32_t args);

void initSyscalls()
{
	Z_envZ___syscall140Z_jii = Z_envZ____syscall140Z_jii = 0;
	Z_envZ___syscall146Z_jii = Z_envZ____syscall146Z_jii = sys_writev;
	Z_envZ___syscall54Z_jii = Z_envZ____syscall54Z_jii = sys_ioctl;
	Z_envZ___syscall6Z_jii = Z_envZ____syscall6Z_jii = 0;
}

#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define TRAP(x) (wasm_rt_trap(WASM_RT_TRAP_##x), 0)

#define MEMCHECK(mem, a, t)  \
  if (UNLIKELY((a) + sizeof(t) > mem->size)) TRAP(OOB)

#define DEFINE_LOAD(name, t1, t2, t3)              \
  static inline t3 name(wasm_rt_memory_t* mem, u64 addr) {   \
    MEMCHECK(mem, addr, t1);                       \
    t1 result;                                     \
    memcpy(&result, &mem->data[addr], sizeof(t1)); \
    return (t3)(t2)result;                         \
  }

#define DEFINE_STORE(name, t1, t2)                           \
  static inline void name(wasm_rt_memory_t* mem, u64 addr, t2 value) { \
    MEMCHECK(mem, addr, t1);                                 \
    t1 wrapped = (t1)value;                                  \
    memcpy(&mem->data[addr], &wrapped, sizeof(t1));          \
  }

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;

DEFINE_LOAD(i32_load, u32, u32, u32);
DEFINE_LOAD(i64_load, u64, u64, u64);
DEFINE_LOAD(f32_load, f32, f32, f32);
DEFINE_LOAD(f64_load, f64, f64, f64);
DEFINE_LOAD(i32_load8_s, s8, s32, u32);
DEFINE_LOAD(i64_load8_s, s8, s64, u64);
DEFINE_LOAD(i32_load8_u, u8, u32, u32);
DEFINE_LOAD(i64_load8_u, u8, u64, u64);
DEFINE_LOAD(i32_load16_s, s16, s32, u32);
DEFINE_LOAD(i64_load16_s, s16, s64, u64);
DEFINE_LOAD(i32_load16_u, u16, u32, u32);
DEFINE_LOAD(i64_load16_u, u16, u64, u64);
DEFINE_LOAD(i64_load32_s, s32, s64, u64);
DEFINE_LOAD(i64_load32_u, u32, u64, u64);
DEFINE_STORE(i32_store, u32, u32);
DEFINE_STORE(i64_store, u64, u64);
DEFINE_STORE(f32_store, f32, f32);
DEFINE_STORE(f64_store, f64, f64);
DEFINE_STORE(i32_store8, u8, u32);
DEFINE_STORE(i32_store16, u16, u32);
DEFINE_STORE(i64_store8, u8, u64);
DEFINE_STORE(i64_store16, u16, u64);
DEFINE_STORE(i64_store32, u32, u64);

struct iovec_custom { char *iov_base; size_t iov_len; };

static uint32_t getArg32(uint32_t& args)
{
	uint32_t ret = i32_load(Z_envZ_memory, args);
	args += 8;
	return ret;
}

static uint64_t getArg64(uint32_t& args)
{
	uint64_t ret = i64_load(Z_envZ_memory, args);
	args += 8;
	return ret;
}

static uintptr_t getUnsandboxedPointer(uintptr_t arg, size_t size)
{
	if (UNLIKELY(arg > Z_envZ_memory->size))
	{
		TRAP(OOB);
	}

	if (UNLIKELY((arg + size) > Z_envZ_memory->size))
	{
		TRAP(OOB);
	}

	uint32_t retIndex = (uint32_t)(arg);
	return (uintptr_t) &(Z_envZ_memory->data[retIndex]);
}

uint64_t sys_writev(uint32_t syscallnum, uint32_t args)
{
	uint32_t expectedSyscallNum = 146;
	if(syscallnum != expectedSyscallNum)
	{
		printf("Syscall number mismatch %d, %d", syscallnum, expectedSyscallNum);
		exit(1);
	}

	uint32_t stream = getArg32(args);
	uintptr_t sandboxed_iov = (uintptr_t)getArg64(args);
	uint32_t iovcnt = getArg32(args);

	if(stream != 1)
	{
		printf("Syscall %d. Expected descriptor 1", expectedSyscallNum);
		exit(1);
	}

	struct iovec_custom* iov = (struct iovec_custom*) getUnsandboxedPointer(sandboxed_iov, iovcnt * sizeof(struct iovec_custom));
	uint64_t ret = 0;

	for (uint32_t i = 0; i < iovcnt; i++) {
		struct iovec_custom* curr_iov = &(iov[i]);

		size_t len = curr_iov->iov_len;
		char* iov_base = (char*) getUnsandboxedPointer((uintptr_t) curr_iov->iov_base, len);

		for (size_t j = 0; j < len; j++) {
			putchar(iov_base[j]);
		}
		ret += len;
	}

	return ret;
}

//as implemented in the js runtime
uint64_t sys_ioctl(uint32_t syscallnum, uint32_t args)
{
	uint32_t expectedSyscallNum = 54;
	if(syscallnum != expectedSyscallNum)
	{
		printf("Syscall number mismatch %d, %d", syscallnum, expectedSyscallNum);
		exit(1);
	}

	return 0;
}