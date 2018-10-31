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

#ifndef WASM_RT_IMPL_H_
#define WASM_RT_IMPL_H_

#include <setjmp.h>
#include <vector>

#include "wasm-rt.h"

#ifdef __cplusplus
extern "C" {
#endif

jmp_buf* wasm_get_setjmp_buff();
uint32_t wasm_rt_register_func(wasm_rt_anyfunc_t func, uint32_t funcType);
void wasm_ret_unregister_func(uint32_t slotNumber);
void* wasm_rt_get_registered_func(uint32_t slotNumber);
uint32_t wasm_rt_register_func_type_with_lists(void* params, void* results);
void wasm_rt_allocate_table_real(wasm_rt_table_t* table, uint32_t elements, uint32_t max_elements);
uint32_t wasm_is_LLVM_backend();
uint32_t wasm_get_heap_base();

#ifdef __cplusplus
}
#endif

#endif // WASM_RT_IMPL_H_
