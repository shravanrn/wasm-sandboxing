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

#define PAGE_SIZE 65536

typedef struct FuncType {
  wasm_rt_type_t* params;
  wasm_rt_type_t* results;
  uint32_t param_count;
  uint32_t result_count;
} FuncType;

_Thread_local uint32_t wasm_rt_call_stack_depth;

_Thread_local jmp_buf g_jmp_buf;
FuncType* g_func_types;
uint32_t g_func_type_count;

wasm_rt_memory_t *Z_envZ_memory;
uint32_t *Z_envZ_memoryBaseZ_i;

wasm_rt_table_t *Z_envZ_table;
uint32_t *Z_envZ_tableBaseZ_i;

uint32_t *Z_envZ_STACKTOPZ_i;
uint32_t *Z_envZ_STACK_MAXZ_i;

uint32_t *Z_envZ_DYNAMICTOP_PTRZ_i;

void (*Z_envZ_abortZ_vi)(uint32_t);
uint32_t (*Z_envZ_abortOnCannotGrowMemoryZ_iv)();
void (*Z_envZ_abortStackOverflowZ_vi)(uint32_t);
void (*Z_envZ____setErrNoZ_vi)(uint32_t);
uint32_t (*Z_envZ_enlargeMemoryZ_iv)();
uint32_t (*Z_envZ_getTotalMemoryZ_iv)(void);

void init();
void initModuleSpecificConstants();
extern uint32_t* STATIC_BUMP;
extern uint32_t (*_E___errno_location)();

void abortCalled(uint32_t param)
{
  printf("WASM module called abort : %u\n", param);
  exit(param);
}

uint32_t abortOnCannotGrowMemoryCalled()
{
  printf("WASM module called abortOnCannotGrowMemory\n");
  exit(1);
}

void abortStackOverflowCalled(uint32_t param)
{
  printf("WASM module called abortStackOverflow : %u\n", param);
  exit(param);
}

void nullFunc_ii(uint32_t param) 
{ 
  printf("Invalid function pointer called with signature 'ii': %u", param);
  exit(param);
}

void nullFunc_iiii(uint32_t param) 
{ 
  printf("Invalid function pointer called with signature 'iiii': %u", param);
  exit(param);
}

void setErrNo(uint32_t value)
{
  uint32_t loc = _E___errno_location();
  Z_envZ_memory->data[loc * 4] = value;
}

uint32_t enlargeMemory() 
{
  abortOnCannotGrowMemoryCalled();
}

uint32_t getTotalMemory()
{
  //4GB
  return 0xFFFFFFFF;
}

#define ALIGN4(val) ((val) + 3) & (-4)

void wasm_init_module()
{
  Z_envZ_abortZ_vi = abortCalled;
  Z_envZ_abortOnCannotGrowMemoryZ_iv = abortOnCannotGrowMemoryCalled;
  Z_envZ_abortStackOverflowZ_vi = abortStackOverflowCalled;
  Z_envZ____setErrNoZ_vi = setErrNo;
  Z_envZ_enlargeMemoryZ_iv = enlargeMemory;
  Z_envZ_getTotalMemoryZ_iv = getTotalMemory;

  Z_envZ_memoryBaseZ_i = (uint32_t *) malloc(sizeof(uint32_t));
  *Z_envZ_memoryBaseZ_i = 1024u;

  Z_envZ_memory = (wasm_rt_memory_t *) malloc(sizeof(wasm_rt_memory_t));
  memset(Z_envZ_memory, 0, sizeof(wasm_rt_memory_t));
  wasm_rt_allocate_memory(Z_envZ_memory, 1, 32768);

  Z_envZ_tableBaseZ_i = (uint32_t *) malloc(sizeof(uint32_t));
  *Z_envZ_tableBaseZ_i = 0;

  Z_envZ_table = (wasm_rt_table_t *) malloc(sizeof(wasm_rt_table_t));
  memset(Z_envZ_table, 0, sizeof(wasm_rt_table_t));
  wasm_rt_allocate_table(Z_envZ_table, 0, 1024);

  initModuleSpecificConstants();

  Z_envZ_STACKTOPZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  Z_envZ_STACK_MAXZ_i = (uint32_t*) malloc(sizeof(uint32_t));
  Z_envZ_DYNAMICTOP_PTRZ_i = (uint32_t*) malloc(sizeof(uint32_t));

  uint32_t staticTop = *Z_envZ_memoryBaseZ_i + *STATIC_BUMP;
  *Z_envZ_DYNAMICTOP_PTRZ_i = staticTop;
  *Z_envZ_STACKTOPZ_i = ALIGN4(staticTop + 1);
  uint32_t totalStack = 5242880;
  *Z_envZ_STACK_MAXZ_i = *Z_envZ_memoryBaseZ_i + totalStack;
  init();
}

int wasm_register_trap_setjmp()
{
  return setjmp(g_jmp_buf);
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

uint32_t wasm_rt_register_func_type(uint32_t param_count,
                                    uint32_t result_count,
                                    ...) {
  FuncType func_type;
  func_type.param_count = param_count;
  func_type.params = malloc(param_count * sizeof(wasm_rt_type_t));
  func_type.result_count = result_count;
  func_type.results = malloc(result_count * sizeof(wasm_rt_type_t));

  va_list args;
  va_start(args, result_count);

  uint32_t i;
  for (i = 0; i < param_count; ++i)
    func_type.params[i] = va_arg(args, wasm_rt_type_t);
  for (i = 0; i < result_count; ++i)
    func_type.results[i] = va_arg(args, wasm_rt_type_t);
  va_end(args);

  for (i = 0; i < g_func_type_count; ++i) {
    if (func_types_are_equal(&g_func_types[i], &func_type)) {
      free(func_type.params);
      free(func_type.results);
      return i + 1;
    }
  }

  uint32_t idx = g_func_type_count++;
  g_func_types = realloc(g_func_types, g_func_type_count * sizeof(FuncType));
  g_func_types[idx] = func_type;
  return idx + 1;
}

void wasm_rt_allocate_memory(wasm_rt_memory_t* memory,
                             uint32_t initial_pages,
                             uint32_t max_pages) {
  memory->pages = initial_pages;
  memory->max_pages = max_pages;
  memory->size = initial_pages * PAGE_SIZE;
  memory->data = calloc(memory->size, 1);
}

uint32_t wasm_rt_grow_memory(wasm_rt_memory_t* memory, uint32_t delta) {
  uint32_t old_pages = memory->pages;
  uint32_t new_pages = memory->pages + delta;
  if (new_pages < old_pages || new_pages > memory->max_pages) {
    return (uint32_t)-1;
  }
  memory->data = realloc(memory->data, new_pages);
  memory->pages = new_pages;
  memory->size = new_pages * PAGE_SIZE;
  return old_pages;
}

void wasm_rt_allocate_table(wasm_rt_table_t* table,
                            uint32_t elements,
                            uint32_t max_elements) {
  table->size = elements;
  table->max_size = max_elements;
  table->data = calloc(table->size, sizeof(wasm_rt_elem_t));
}
