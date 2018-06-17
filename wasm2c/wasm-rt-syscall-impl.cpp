#include <stdint.h>

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

void initSyscalls()
{
	Z_envZ___syscall140Z_jii = Z_envZ____syscall140Z_jii = 0;
	Z_envZ___syscall146Z_jii = Z_envZ____syscall146Z_jii = 0;
	Z_envZ___syscall54Z_jii = Z_envZ____syscall54Z_jii = 0;
	Z_envZ___syscall6Z_jii = Z_envZ____syscall6Z_jii = 0;
}