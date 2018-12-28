/*
 * Copyright 2018 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-rt-impl.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <inttypes.h>
#include <math.h>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <sys/types.h>

#define PAGE_SIZE 65536
#define FUNC_TABLE_SIZE 1024

typedef struct FuncType {
  wasm_rt_type_t* params;
  wasm_rt_type_t* results;
  uint32_t param_count;
  uint32_t result_count;
} FuncType;

#ifdef __cplusplus
thread_local uint32_t wasm_rt_call_stack_depth;
thread_local jmp_buf g_jmp_buf;
#else
_Thread_local uint32_t wasm_rt_call_stack_depth;
_Thread_local jmp_buf g_jmp_buf;
#endif

#define _NSIG 65

struct pthread_copy {
// XXX Emscripten: Need some custom thread control structures.
	// Note: The specific order of these fields is important, since these are accessed
	// by direct pointer arithmetic in pthread-main.js.
	int threadStatus; // 0: thread not exited, 1: exited.
	int threadExitCode; // Thread exit code.
	int tempDoublePtr[3]; // Temporary memory area for double operations in runtime.
	void *profilerBlock; // If --threadprofiling is enabled, this pointer is allocated to contain internal information about the thread state for profiling purposes.

	struct pthread_copy *self;
	void **dtv, *unused1, *unused2;
	uint32_t sysinfo;
	uint32_t canary, canary2;
	uint32_t tid, pid;
	int tsd_used, errno_val;
	volatile int cancel, canceldisable, cancelasync;
	int detached;
	unsigned char *map_base;
	uint32_t map_size;
	void *stack;
	uint32_t stack_size;
	void *start_arg;
	void *(*start)(void *);
	void *result;
	struct __ptcb *cancelbuf;
	void **tsd;
	pthread_attr_t attr;
	volatile int dead;
	struct {
		volatile void *volatile head;
		long off;
		volatile void *volatile pending;
	} robust_list;
	int unblock_cancel;
	volatile int timer_id;
	locale_t locale;
	volatile int killlock[2];
	volatile int exitlock[2];
	volatile int startlock[2];
	unsigned long sigmask[_NSIG/8/sizeof(long)];
	char *dlerror_buf;
	int dlerror_flag;
	void *stdio_locks;
	uint32_t canary_at_end;
	void **dtv_copy;
};



FuncType* g_func_types;
uint32_t g_func_type_count;

wasm_rt_memory_t *Z_envZ_memory;
uint32_t *Z_envZ_memoryBaseZ_i;

wasm_rt_table_t *Z_envZ_table;
uint32_t *Z_envZ_tableBaseZ_i;

uint32_t *Z_envZ_STACKTOPZ_i;
uint32_t *Z_envZ_STACK_MAXZ_i;

uint32_t *Z_envZ_DYNAMICTOP_PTRZ_i;

uint32_t *Z_envZ_tempDoublePtrZ_i;

double *Z_globalZ_NaNZ_d;
double *Z_globalZ_InfinityZ_d;
uint32_t *Z_envZ_ABORTZ_i;

void (*Z_envZ_abortZ_vv)(void);
void (*Z_envZ_abortZ_vi)(uint32_t);
uint32_t (*Z_envZ_abortOnCannotGrowMemoryZ_iv)();
void (*Z_envZ_abortStackOverflowZ_vi)(uint32_t);
void (*Z_envZ____setErrNoZ_vi)(uint32_t);
uint32_t (*Z_envZ_enlargeMemoryZ_iv)();
uint32_t (*Z_envZ_getTotalMemoryZ_iv)(void);
void (*Z_envZ___lockZ_vi)(uint32_t);
void (*Z_envZ___unlockZ_vi)(uint32_t);
uint32_t (*Z_envZ__emscripten_memcpy_bigZ_iiii)(uint32_t, uint32_t, uint32_t);
void (*Z_envZ_nullFunc_XZ_vi)(uint32_t);
void (*Z_envZ_nullFunc_iiZ_vi)(uint32_t);
void (*Z_envZ_nullFunc_iiiiZ_vi)(uint32_t);
void (*Z_envZ_invoke_viZ_vii)(uint32_t, uint32_t);
void (*Z_envZ_invoke_viiZ_viii)(uint32_t, uint32_t, uint32_t);
void (*Z_envZ_invoke_viiiZ_viiii)(uint32_t, uint32_t, uint32_t, uint32_t);
void (*Z_envZ_invoke_viiiiZ_viiiii)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_invoke_iiZ_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_invoke_iiiZ_iiii)(uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_invoke_iiiiZ_iiiii)(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_invoke_iiiiiZ_iiiiii)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_sbrkZ_ii)(uint32_t);
void (*Z_envZ_exitZ_vi)(uint32_t);
uint32_t (*Z_envZ_getenvZ_ii)(uint32_t);
void (*Z_envZ___buildEnvironmentZ_vi)(uint32_t);
double (*Z_envZ_fabsZ_dd)(double);
uint32_t (*Z_envZ_gmtimeZ_ii)(uint32_t);
double (*Z_envZ_sqrtZ_dd)(double);
uint32_t (*Z_envZ_testSetjmpZ_iiii)(uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_saveSetjmpZ_iiiii)(uint32_t, uint32_t, uint32_t, uint32_t);
void (*Z_envZ_emscripten_longjmpZ_vii)(uint32_t, uint32_t);

//Threading
uint32_t (*Z_envZ_emscripten_atomic_cas_u32Z_iiii)(uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_atomic_add_u32Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_futex_wakeZ_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_pthread_selfZ_iv)(void);
uint32_t (*Z_envZ_emscripten_atomic_sub_u32Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_atomic_load_u32Z_ii)(uint32_t);
uint32_t (*Z_envZ_emscripten_atomic_store_u32Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ___clock_gettimeZ_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_is_main_runtime_threadZ_iv)(void);
uint32_t (*Z_envZ_emscripten_futex_waitZ_iiid)(uint32_t, uint32_t, double);
void (*Z_envZ_emscripten_conditional_set_current_thread_statusZ_vii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_has_threading_supportZ_iv)(void);
uint32_t (*Z_envZ_pthread_createZ_iiiii)(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ___call_mainZ_iii)(uint32_t, uint32_t);
void (*Z_envZ_emscripten_set_thread_nameZ_vii)(uint32_t, uint32_t);
double (*Z_envZ_emscripten_get_nowZ_dv)(void);
void (*Z_envZ_emscripten_set_current_thread_statusZ_vi)(uint32_t);
void (*Z_envZ___assert_failZ_viiii)(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_syscallZ_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_gettimeofdayZ_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_atomic_exchange_u32Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ_emscripten_asm_const_iiiZ_iiii)(uint32_t, uint32_t, uint32_t);

void notImplemented(){
  printf("Not implemented\n");
  abort();
}

extern "C" {
void init();
}
void initSyscalls();
void initModuleSpecificConstants();
extern uint32_t* STATIC_BUMP;
using ErrNoType = uint32_t (*)();
ErrNoType errno_location;
ErrNoType __attribute__((weak)) _E___errno_location = 0;
ErrNoType __attribute__((weak)) _E__errno_location = 0;


#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define TRAP(x) (wasm_rt_trap(WASM_RT_TRAP_##x), 0)

#define MEMCHECK(mem, a, t)  \
  if (UNLIKELY((a) + sizeof(t) > mem->size)) TRAP(OOB)

#define DEFINE_LOAD(name, t1, t2, t3)              \
  static inline t3 name(wasm_rt_memory_t* mem, uint64_t addr) {   \
    MEMCHECK(mem, addr, t1);                       \
    t1 result;                                     \
    memcpy(&result, &mem->data[addr], sizeof(t1)); \
    return (t3)(t2)result;                         \
  }

#define DEFINE_STORE(name, t1, t2)                           \
  static inline void name(wasm_rt_memory_t* mem, uint64_t addr, t2 value) { \
    MEMCHECK(mem, addr, t1);                                 \
    t1 wrapped = (t1)value;                                  \
    memcpy(&mem->data[addr], &wrapped, sizeof(t1));          \
  }

DEFINE_LOAD(i32_load, uint32_t, uint32_t, uint32_t);
DEFINE_STORE(i32_store, uint32_t, uint32_t);

void writeStackCookie() {
  assert((*Z_envZ_STACK_MAXZ_i & 3) == 0);
  i32_store(Z_envZ_memory, *Z_envZ_STACK_MAXZ_i - 4, 0x02135467);
  i32_store(Z_envZ_memory, *Z_envZ_STACK_MAXZ_i - 8, 0x89BACDFE);
}

void checkStackCookie() {
  uint32_t c1 = i32_load(Z_envZ_memory, *Z_envZ_STACK_MAXZ_i - 4);
  uint32_t c2 = i32_load(Z_envZ_memory, *Z_envZ_STACK_MAXZ_i - 8);
  if (c1 != 0x02135467 || c2 != 0x89BACDFE) {
    printf("Stack overflow! Stack cookie has been overwritten, expected hex dwords 0x89BACDFE and 0x02135467, but received %d %d\n",
      c1, c2);
      abort();
  }
  // Also test the global address 0 for integrity. This check is not compatible with SAFE_SPLIT_MEMORY though, since that mode already tests all address 0 accesses on its own.
  uint32_t nc = i32_load(Z_envZ_memory, 0);
  if (nc != 0x63736d65 /* 'emsc' */) {
    printf("Runtime error: The application has corrupted its heap memory area (address zero)!\n");
    abort();
  }
}

void abortCalledVoid()
{
  printf("WASM module called abort\n");
  abort();
}

void abortCalled(uint32_t param)
{
  printf("WASM module called abort : %u\n", param);
  abort();
}

uint32_t abortOnCannotGrowMemoryCalled()
{
  printf("WASM module called abortOnCannotGrowMemory\n");
  abort();
}

void abortStackOverflowCalled(uint32_t param)
{
  printf("WASM module called abortStackOverflow : %u\n", param);
  abort();
}

void nullFunc_X(uint32_t param)
{
  printf("Invalid function pointer called with signature 'X': %u\n", param);
  abort();
}

void nullFunc_ii(uint32_t param) 
{ 
  printf("Invalid function pointer called with signature 'ii': %u\n", param);
  abort();
}

void nullFunc_iiii(uint32_t param) 
{ 
  printf("Invalid function pointer called with signature 'iiii': %u\n", param);
  abort();
}

void (*_EdynCall_vi)(uint32_t p0, uint32_t p1);
void (*_EdynCall_vii)(uint32_t p0, uint32_t p1, uint32_t p2);
void (*_EdynCall_viii)(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3);
void (*_EdynCall_viiii)(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);
uint32_t (*_EdynCall_ii)(uint32_t p0, uint32_t p1);
uint32_t (*_EdynCall_iii)(uint32_t p0, uint32_t p1, uint32_t p2);
uint32_t (*_EdynCall_iiii)(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3);
uint32_t (*_EdynCall_iiiii)(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);

void invoke_vi(uint32_t p0, uint32_t p1)
{
  return _EdynCall_vi(p0, p1);
}
void invoke_vii(uint32_t p0, uint32_t p1, uint32_t p2)
{
  return _EdynCall_vii(p0, p1, p2);
}
void invoke_viii(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3)
{
  return _EdynCall_viii(p0, p1, p2, p3);
}
void invoke_viiii(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
  return _EdynCall_viiii(p0, p1, p2, p3, p4);
}
uint32_t invoke_ii(uint32_t p0, uint32_t p1)
{
  return _EdynCall_ii(p0, p1);
}
uint32_t invoke_iii(uint32_t p0, uint32_t p1, uint32_t p2)
{
  return _EdynCall_iii(p0, p1, p2);
}
uint32_t invoke_iiii(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3)
{
  return _EdynCall_iiii(p0, p1, p2, p3);
}
uint32_t invoke_iiiii(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
  return _EdynCall_iiiii(p0, p1, p2, p3, p4);
}

void setErrNo(uint32_t value)
{
  uint32_t loc = errno_location();
  i32_store(Z_envZ_memory, loc, value);
}

uint32_t enlargeMemory() 
{
  return abortOnCannotGrowMemoryCalled();
}

uint32_t getTotalMemory()
{
  //2GB - 1 page
  return (uint32_t)(((uint64_t)0x80000000) - PAGE_SIZE);
}

uint32_t memcpy_big(uint32_t dest, uint32_t src, uint32_t num) {
  memcpy(&(Z_envZ_memory->data[dest]), &(Z_envZ_memory->data[src]), num);
  return dest;
} 

std::map<uint32_t, std::mutex> lockMap;

void lockImpl(uint32_t lockId)
{
  lockMap[lockId].lock();
}

void unlockImpl(uint32_t lockId)
{
  lockMap[lockId].unlock();
}

uint32_t sbrk_impl(uint32_t increment_unsigned) {

  int32_t increment = increment_unsigned; //fix sign
  int32_t oldDynamicTop = i32_load(Z_envZ_memory, *Z_envZ_DYNAMICTOP_PTRZ_i);
  int32_t newDynamicTop = oldDynamicTop + increment;

  if ((increment > 0 && newDynamicTop < oldDynamicTop) // Detect and fail if we would wrap around signed 32-bit int.
    || newDynamicTop < 0) { // Also underflow, sbrk() should be able to be used to subtract.
    return abortOnCannotGrowMemoryCalled();
    // setErrNo(12);
    // return -1;
  }

  i32_store(Z_envZ_memory, *Z_envZ_DYNAMICTOP_PTRZ_i, newDynamicTop);
  int32_t totalMemory = getTotalMemory();
  if (newDynamicTop > totalMemory) {
    if (enlargeMemory() == 0) {
      i32_store(Z_envZ_memory, *Z_envZ_DYNAMICTOP_PTRZ_i, oldDynamicTop);
      setErrNo(12);
      return -1;
    }
  }

  return oldDynamicTop;
}

void getErrLocation()
{
  errno_location = _E__errno_location;
  if(!errno_location)
  {
    errno_location = _E___errno_location;
    if(!errno_location)
    {
      printf("Error occurred trying to find the __errno_location symbol\n");
      abort();
    }
  }
}

void buildEnvironment(uint32_t ptr)
{
  //for now we don't support environments
  i32_store(Z_envZ_memory, ptr, 0);
}

uint32_t getenv_impl(uint32_t name)
{
  //for now we don't support looking at environment variables
  return 0;
}

uint32_t gmtime_impl(uint32_t)
{
  printf("gmtime not implemented\n");
  abort();
}

uint32_t STACK_ALIGN = 16;

static uint32_t staticAlloc(uint32_t* STATICTOP, uint32_t size) {
  uint32_t ret = *STATICTOP;
  *STATICTOP = (*STATICTOP + size + 15) & -16;
  assert((*STATICTOP < getTotalMemory()) && "not enough memory for static allocation - increase TOTAL_MEMORY");
  return ret;
}

static uint32_t alignMemory(uint32_t size) {
  uint32_t ret = ((uint32_t)ceil(size / STACK_ALIGN)) * STACK_ALIGN;
  return ret;
}

uint32_t testSetjmp(uint32_t id, uint32_t table, uint32_t size) {
  printf("testSetjmp not implemented\n");
  abort();
}

void (*_EsetTempRet0)(uint32_t);
uint32_t (*_Erealloc)(uint32_t, uint32_t);

std::atomic<uint32_t> setjmpId(0);

uint32_t saveSetjmp(uint32_t env, uint32_t label, uint32_t table, uint32_t size)
{
  uint32_t newValue = ++setjmpId;
  i32_store(Z_envZ_memory, env, newValue);

  for(uint32_t i = 0; i < size; i++) {
    auto loc = table+(i<<3);
    if (i32_load(Z_envZ_memory, loc) == 0) {
      i32_store(Z_envZ_memory, loc, newValue);
      i32_store(Z_envZ_memory, loc + 4, label);
      // prepare next slot
      i32_store(Z_envZ_memory, loc + 8, 0);
      _EsetTempRet0(size);
      return table;
    }
  }
  // grow the table
  size = size * 2;
  table = _Erealloc(table, 8 * (size+1));
  table = saveSetjmp(env, label, table, size);
  _EsetTempRet0(size);
  return table;
}

void emscripten_longjmp(uint32_t env, uint32_t value) 
{
  printf("emscripten_longjmp not implemented\n");
  abort();
}

void (*_EstackRestore)(uint32_t);
uint32_t (*_EstackSave)(void);

static void wasm_set_constants()
{
  initModuleSpecificConstants();
  uint32_t TOTAL_STACK = 5242880;
  uint32_t GLOBAL_BASE = 1024;
  uint32_t STATIC_BASE = GLOBAL_BASE;
  uint32_t STATICTOP = STATIC_BASE + *STATIC_BUMP;
  uint32_t tempDoublePtr = STATICTOP; STATICTOP += 16;
  assert(tempDoublePtr % 8 == 0);
  uint32_t DYNAMICTOP_PTR = staticAlloc(&STATICTOP, 4);
  uint32_t STACK_BASE, STACKTOP;
  STACK_BASE = STACKTOP = alignMemory(STATICTOP);
  uint32_t STACK_MAX = STACK_BASE + TOTAL_STACK;
  uint32_t DYNAMIC_BASE = alignMemory(STACK_MAX);
  i32_store(Z_envZ_memory, DYNAMICTOP_PTR, DYNAMIC_BASE);
  assert(DYNAMIC_BASE < getTotalMemory() && "TOTAL_MEMORY not big enough for stack");

  // STACKTOP = STACK_BASE + TOTAL_STACK;
  // STACK_MAX = STACK_BASE;

  Z_envZ_STACKTOPZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  Z_envZ_STACK_MAXZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  Z_envZ_DYNAMICTOP_PTRZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  Z_envZ_tempDoublePtrZ_i = (uint32_t*) malloc(sizeof(uint32_t));

  *Z_envZ_STACKTOPZ_i = STACKTOP;
  *Z_envZ_STACK_MAXZ_i = STACK_MAX;
  *Z_envZ_DYNAMICTOP_PTRZ_i = DYNAMICTOP_PTR;
  *Z_envZ_tempDoublePtrZ_i = tempDoublePtr;
}

static void wasm_set_stack()
{
  uint32_t stack = _EstackSave();
  //wasm-ld loader sets its own stack location, while wasm2s does not
  //to support both cases, set stack only if it isnt already set
  if(!stack)
  {
    _EstackRestore(*Z_envZ_STACKTOPZ_i);
  }
}

static std::mutex atomic_mutex;
uint32_t Z_envZ_emscripten_atomic_cas_u32Z_iiii_impl(uint32_t ref, uint32_t expected, uint32_t desired)
{
  std::lock_guard<std::mutex> lock(atomic_mutex);
  uint32_t val = i32_load(Z_envZ_memory, ref);
  if (val == expected)
  {
    i32_store(Z_envZ_memory, ref, desired);
    return val;
  }
  //return different value
  return val - 1;
}
uint32_t Z_envZ_emscripten_atomic_add_u32Z_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_atomic_add_u32Z_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_futex_wakeZ_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_futex_wakeZ_iii not implemented\n");
  abort();
}

uint32_t (*_Emalloc)(uint32_t);
static std::mutex pthread_self_mutex;
uint32_t Z_envZ_pthread_selfZ_iv_impl(void)
{
  uint32_t threadDataLoc = _Emalloc(sizeof(struct pthread_copy));
  struct pthread_copy* threadData = (struct pthread_copy*) &(Z_envZ_memory->data[threadDataLoc]);
  memset(threadData, 0, sizeof(struct pthread_copy));

  threadData->self = (struct pthread_copy*) (uintptr_t) threadDataLoc;
  threadData->robust_list.head = &threadData->robust_list.head;

  uint32_t tlsMemoryLoc = _Emalloc(128 * 4);
  void* tlsMemory =  &(Z_envZ_memory->data[tlsMemoryLoc]);
  memset(tlsMemory, 9, 128 * 4);

  {
    std::lock_guard<std::mutex> lock(pthread_self_mutex);
    // Atomics.store(HEAPU32, (PThread.mainThreadBlock + 176 ) >> 2, tlsMemory); // Init thread-local-storage memory array.
    threadData->tsd = (void**) (uintptr_t) tlsMemoryLoc;
    // Atomics.store(HEAPU32, (PThread.mainThreadBlock + 76 ) >> 2, PThread.mainThreadBlock); // Main thread ID.
    threadData->tid = threadDataLoc;
    // Atomics.store(HEAPU32, (PThread.mainThreadBlock + 80 ) >> 2, PROCINFO.pid); // Process ID.
    threadData->pid = 42;
  }

  return threadDataLoc;
}
uint32_t Z_envZ_emscripten_atomic_sub_u32Z_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_atomic_sub_u32Z_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_atomic_load_u32Z_ii_impl(uint32_t ref)
{
  std::lock_guard<std::mutex> lock(atomic_mutex);
  return i32_load(Z_envZ_memory, ref);
}
uint32_t Z_envZ_emscripten_atomic_store_u32Z_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_atomic_store_u32Z_iii not implemented\n");
  abort();
}
uint32_t Z_envZ___clock_gettimeZ_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ___clock_gettimeZ_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_is_main_runtime_threadZ_iv_impl(void)
{
  printf("Z_envZ_emscripten_is_main_runtime_threadZ_iv not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_futex_waitZ_iiid_impl(uint32_t, uint32_t, double)
{
  printf("Z_envZ_emscripten_futex_waitZ_iiid not implemented\n");
  abort();
}
void Z_envZ_emscripten_conditional_set_current_thread_statusZ_vii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_conditional_set_current_thread_statusZ_vii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_has_threading_supportZ_iv_impl(void)
{
  printf("Z_envZ_emscripten_has_threading_supportZ_iv not implemented\n");
  abort();
}
uint32_t Z_envZ_pthread_createZ_iiiii_impl(uint32_t, uint32_t, uint32_t, uint32_t)
{
  printf("Z_envZ_pthread_createZ_iiiii not implemented\n");
  abort();
}
uint32_t Z_envZ___call_mainZ_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ___call_mainZ_iii not implemented\n");
  abort();
}
void Z_envZ_emscripten_set_thread_nameZ_vii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_set_thread_nameZ_vii not implemented\n");
  abort();
}
double Z_envZ_emscripten_get_nowZ_dv_impl(void)
{
  printf("Z_envZ_emscripten_get_nowZ_dv not implemented\n");
  abort();
}
void Z_envZ_emscripten_set_current_thread_statusZ_vi_impl(uint32_t)
{
  printf("Z_envZ_emscripten_set_current_thread_statusZ_vi not implemented\n");
  abort();
}
void Z_envZ___assert_failZ_viiii_impl(uint32_t, uint32_t, uint32_t, uint32_t)
{
  printf("Z_envZ___assert_failZ_viiii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_syscallZ_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_syscallZ_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_gettimeofdayZ_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_gettimeofdayZ_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_atomic_exchange_u32Z_iii_impl(uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_atomic_exchange_u32Z_iii not implemented\n");
  abort();
}
uint32_t Z_envZ_emscripten_asm_const_iiiZ_iiii_impl(uint32_t, uint32_t, uint32_t)
{
  printf("Z_envZ_emscripten_asm_const_iiiZ_iiii not implemented\n");
  abort();
}

void wasm_rt_allocate_memory(wasm_rt_memory_t* memory, uint32_t initial_pages, uint32_t max_pages);
void (*_E__wasm_call_ctors)(void);

void wasm_init_module()
{
  Z_envZ_abortZ_vv = abortCalledVoid;
  Z_envZ_abortZ_vi = abortCalled;
  Z_envZ_abortOnCannotGrowMemoryZ_iv = abortOnCannotGrowMemoryCalled;
  Z_envZ_abortStackOverflowZ_vi = abortStackOverflowCalled;
  Z_envZ____setErrNoZ_vi = setErrNo;
  Z_envZ_enlargeMemoryZ_iv = enlargeMemory;
  Z_envZ_getTotalMemoryZ_iv = getTotalMemory;
  Z_envZ___lockZ_vi = lockImpl;
  Z_envZ___unlockZ_vi = unlockImpl;
  Z_envZ__emscripten_memcpy_bigZ_iiii = memcpy_big;
  Z_envZ_nullFunc_XZ_vi = nullFunc_X;
  Z_envZ_nullFunc_iiZ_vi = nullFunc_ii;
  Z_envZ_nullFunc_iiiiZ_vi = nullFunc_iiii;
  Z_envZ_invoke_viZ_vii = invoke_vi;
  Z_envZ_invoke_viiZ_viii = invoke_vii;
  Z_envZ_invoke_viiiZ_viiii = invoke_viii;
  Z_envZ_invoke_viiiiZ_viiiii = invoke_viiii;
  Z_envZ_invoke_iiZ_iii = invoke_ii;
  Z_envZ_invoke_iiiZ_iiii = invoke_iii;
  Z_envZ_invoke_iiiiZ_iiiii = invoke_iiii;
  Z_envZ_invoke_iiiiiZ_iiiiii = invoke_iiiii;
  Z_envZ_sbrkZ_ii = sbrk_impl;
  Z_envZ_exitZ_vi = (decltype(Z_envZ_exitZ_vi))exit;
  Z_envZ_getenvZ_ii = getenv_impl;
  Z_envZ___buildEnvironmentZ_vi = buildEnvironment;
  Z_envZ_fabsZ_dd = fabs;
  Z_envZ_gmtimeZ_ii = gmtime_impl;
  Z_envZ_sqrtZ_dd = sqrt;
  Z_envZ_testSetjmpZ_iiii = testSetjmp;
  Z_envZ_saveSetjmpZ_iiiii = saveSetjmp;
  Z_envZ_emscripten_longjmpZ_vii = emscripten_longjmp;

  Z_envZ_emscripten_atomic_cas_u32Z_iiii = Z_envZ_emscripten_atomic_cas_u32Z_iiii_impl;
  Z_envZ_emscripten_atomic_add_u32Z_iii = Z_envZ_emscripten_atomic_add_u32Z_iii_impl;
  Z_envZ_emscripten_futex_wakeZ_iii = Z_envZ_emscripten_futex_wakeZ_iii_impl;
  Z_envZ_pthread_selfZ_iv = Z_envZ_pthread_selfZ_iv_impl;
  Z_envZ_emscripten_atomic_sub_u32Z_iii = Z_envZ_emscripten_atomic_sub_u32Z_iii_impl;
  Z_envZ_emscripten_atomic_load_u32Z_ii = Z_envZ_emscripten_atomic_load_u32Z_ii_impl;
  Z_envZ_emscripten_atomic_store_u32Z_iii = Z_envZ_emscripten_atomic_store_u32Z_iii_impl;
  Z_envZ___clock_gettimeZ_iii = Z_envZ___clock_gettimeZ_iii_impl;
  Z_envZ_emscripten_is_main_runtime_threadZ_iv = Z_envZ_emscripten_is_main_runtime_threadZ_iv_impl;
  Z_envZ_emscripten_futex_waitZ_iiid = Z_envZ_emscripten_futex_waitZ_iiid_impl;
  Z_envZ_emscripten_conditional_set_current_thread_statusZ_vii = Z_envZ_emscripten_conditional_set_current_thread_statusZ_vii_impl;
  Z_envZ_emscripten_has_threading_supportZ_iv = Z_envZ_emscripten_has_threading_supportZ_iv_impl;
  Z_envZ_pthread_createZ_iiiii = Z_envZ_pthread_createZ_iiiii_impl;
  Z_envZ___call_mainZ_iii = Z_envZ___call_mainZ_iii_impl;
  Z_envZ_emscripten_set_thread_nameZ_vii = Z_envZ_emscripten_set_thread_nameZ_vii_impl;
  Z_envZ_emscripten_get_nowZ_dv = Z_envZ_emscripten_get_nowZ_dv_impl;
  Z_envZ_emscripten_set_current_thread_statusZ_vi = Z_envZ_emscripten_set_current_thread_statusZ_vi_impl;
  Z_envZ___assert_failZ_viiii = Z_envZ___assert_failZ_viiii_impl;
  Z_envZ_emscripten_syscallZ_iii = Z_envZ_emscripten_syscallZ_iii_impl;
  Z_envZ_gettimeofdayZ_iii = Z_envZ_gettimeofdayZ_iii_impl;
  Z_envZ_emscripten_atomic_exchange_u32Z_iii = Z_envZ_emscripten_atomic_exchange_u32Z_iii_impl;
  Z_envZ_emscripten_asm_const_iiiZ_iiii = Z_envZ_emscripten_asm_const_iiiZ_iiii_impl;

  Z_envZ_memoryBaseZ_i = (uint32_t *) malloc(sizeof(uint32_t));
  *Z_envZ_memoryBaseZ_i = 1024u;

  Z_envZ_memory = (wasm_rt_memory_t *) malloc(sizeof(wasm_rt_memory_t));
  memset(Z_envZ_memory, 0, sizeof(wasm_rt_memory_t));
  uint32_t pagesToAllocate = getTotalMemory() / PAGE_SIZE;
  wasm_rt_allocate_memory(Z_envZ_memory, pagesToAllocate, pagesToAllocate);

  Z_envZ_tableBaseZ_i = (uint32_t *) malloc(sizeof(uint32_t));
  *Z_envZ_tableBaseZ_i = 0;

  Z_envZ_table = (wasm_rt_table_t *) malloc(sizeof(wasm_rt_table_t));
  memset(Z_envZ_table, 0, sizeof(wasm_rt_table_t));
  wasm_rt_allocate_table_real(Z_envZ_table, FUNC_TABLE_SIZE, FUNC_TABLE_SIZE);

  Z_globalZ_NaNZ_d = (double*) malloc(sizeof(double));
  Z_globalZ_InfinityZ_d = (double*) malloc(sizeof(double));

  *Z_globalZ_NaNZ_d = strtod("NaN", NULL);
  *Z_globalZ_InfinityZ_d = strtod("Inf", NULL);

  Z_envZ_ABORTZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  *Z_envZ_ABORTZ_i = 0;

  initSyscalls();

  wasm_set_constants();

  init();
  
  wasm_set_stack();

  getErrLocation();

  i32_store(Z_envZ_memory, 0, 0x63736d65);

  writeStackCookie();
  checkStackCookie();

  _E__wasm_call_ctors();
}

jmp_buf* wasm_get_setjmp_buff()
{
  return &g_jmp_buf;
}

void wasm_rt_trap(wasm_rt_trap_t code) {
  assert(code != WASM_RT_TRAP_NONE);
  longjmp(g_jmp_buf, code);
}

static bool func_types_are_equal(FuncType* a, FuncType* b) {
  if (a->param_count != b->param_count || a->result_count != b->result_count)
    return 0;
  int i;
  for (i = 0; i < a->param_count; ++i)
    if (a->params[i] != b->params[i])
      return 0;
  for (i = 0; i < a->result_count; ++i)
    if (a->results[i] != b->results[i])
      return 0;
  return 1;
}

static std::mutex func_tables_mutex;
uint32_t wasm_rt_register_func_type(uint32_t param_count,
                                    uint32_t result_count,
                                    ...) {
  std::lock_guard<std::mutex> lock(func_tables_mutex);

  FuncType func_type;
  func_type.param_count = param_count;
  func_type.params = (wasm_rt_type_t*) malloc(param_count * sizeof(wasm_rt_type_t));
  func_type.result_count = result_count;
  func_type.results = (wasm_rt_type_t*) malloc(result_count * sizeof(wasm_rt_type_t));
  if(!func_type.params || !func_type.results)
  {
    printf("Failed to allocate register_func_type memory!\n");
    abort();
  }

  va_list args;
  va_start(args, result_count);

  uint32_t i;
  for (i = 0; i < param_count; ++i)
  {
    // func_type.params[i] = va_arg(args, wasm_rt_type_t);
    func_type.params[i] = (wasm_rt_type_t) va_arg(args, int);
  }
  for (i = 0; i < result_count; ++i)
  {
    // func_type.results[i] = va_arg(args, wasm_rt_type_t);
    func_type.results[i] = (wasm_rt_type_t) va_arg(args, int);
  }
  va_end(args);

  for (i = 0; i < g_func_type_count; ++i) {
    if (func_types_are_equal(&g_func_types[i], &func_type)) {
      free(func_type.params);
      free(func_type.results);
      return i + 1;
    }
  }

  uint32_t idx = g_func_type_count++;
  g_func_types = (FuncType*) realloc(g_func_types, g_func_type_count * sizeof(FuncType));
  if(!g_func_types)
  {
    printf("Failed to allocate wasm function types table!\n");
    abort();
  }
  g_func_types[idx] = func_type;
  return idx + 1;
}

uint32_t wasm_rt_register_func_type_with_lists(void* p_params, void* p_results) {
  std::lock_guard<std::mutex> lock(func_tables_mutex);

  std::vector<wasm_rt_type_t>& params = *((std::vector<wasm_rt_type_t> *) p_params);
  std::vector<wasm_rt_type_t>& results = *((std::vector<wasm_rt_type_t> *) p_results);

  FuncType func_type;
  func_type.param_count = params.size();

  if(func_type.param_count != 0)
  {
    func_type.params = (wasm_rt_type_t*) malloc(func_type.param_count * sizeof(wasm_rt_type_t));
    if(!func_type.params)
    {
      printf("Failed to allocate register_func_type memory!\n");
      abort();
    }
  }
  else
  {
    func_type.params = 0;
  }

  func_type.result_count = results.size();
  if(func_type.result_count != 0)
  {
    func_type.results = (wasm_rt_type_t*) malloc(func_type.result_count * sizeof(wasm_rt_type_t));
    if(!func_type.results)
    {
      printf("Failed to allocate register_func_type memory!\n");
      abort();
    }
  }
  else
  {
    func_type.results = 0;
  }

  uint32_t i;
  for (i = 0; i < func_type.param_count; ++i)
  {
    func_type.params[i] = params[i];
  }
  for (i = 0; i < func_type.result_count; ++i)
  {
    func_type.results[i] = results[i];
  }

  for (i = 0; i < g_func_type_count; ++i) {
    if (func_types_are_equal(&g_func_types[i], &func_type)) {
      if(func_type.params)
      {
        free(func_type.params);
      }
      if(func_type.results)
      {
        free(func_type.results);
      }
      return i + 1;
    }
  }

  uint32_t idx = g_func_type_count++;
  g_func_types = (FuncType*) realloc(g_func_types, g_func_type_count * sizeof(FuncType));
  if(!g_func_types)
  {
    printf("Failed to allocate wasm function types table!\n");
    abort();
  }
  g_func_types[idx] = func_type;
  return idx + 1;
}

uint32_t wasm_rt_register_func(wasm_rt_anyfunc_t func, uint32_t funcType)
{
  std::lock_guard<std::mutex> lock(func_tables_mutex);

  for(uint32_t i = 1; i < FUNC_TABLE_SIZE; i++)
  {
    //find empty slot
    if(!Z_envZ_table->data[i].func)
    {
      Z_envZ_table->data[i].func = func;
      Z_envZ_table->data[i].func_type = funcType;
      return i;
    }
  }

  printf("Register functions ran out of slots\n");
  abort();
}

void wasm_ret_unregister_func(uint32_t slotNumber)
{
  std::lock_guard<std::mutex> lock(func_tables_mutex);

  if(slotNumber < FUNC_TABLE_SIZE)
  {
    Z_envZ_table->data[slotNumber].func = 0;
    Z_envZ_table->data[slotNumber].func_type = 0;
  }
}

void* wasm_rt_get_registered_func(uint32_t slotNumber)
{
  std::lock_guard<std::mutex> lock(func_tables_mutex);

  if(slotNumber < FUNC_TABLE_SIZE)
  {
    return (void*) Z_envZ_table->data[slotNumber].func;
  }

  return nullptr;
}

void wasm_rt_allocate_memory(wasm_rt_memory_t* memory,
                             uint32_t initial_pages,
                             uint32_t max_pages) {
  memory->pages = initial_pages;
  memory->max_pages = max_pages;
  memory->size = initial_pages * PAGE_SIZE;
  memory->data = (uint8_t*) calloc(memory->size, 1);
  if(!memory->data)
  {
    printf("Failed to allocate sandbox memory!\n");
    abort();
  }
}

void wasm_rt_allocate_table_real(wasm_rt_table_t* table,
                            uint32_t elements,
                            uint32_t max_elements) {
  table->size = elements;
  table->max_size = max_elements;
  //calloc zero initializes automatically
  table->data = (wasm_rt_elem_t*) calloc(table->size, sizeof(wasm_rt_elem_t));
  if(!table->data)
  {
    printf("Failed to allocate sandbox Wasm table!\n");
    abort();
  }
}

//basically a no-op
void wasm_rt_allocate_table(wasm_rt_table_t* table,
                            uint32_t elements,
                            uint32_t max_elements) {
  //make sure the internal function table is the same as the external function table
  *table = *Z_envZ_table;
}

uint32_t wasm_rt_grow_memory(wasm_rt_memory_t*, uint32_t pages)
{
  printf("Grow memory not supported!\n");
  abort();
}

extern uint32_t* Z___heap_baseZ_i;

uint32_t wasm_get_heap_base()
{
  return *Z___heap_baseZ_i;
}