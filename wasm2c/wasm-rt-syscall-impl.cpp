#include <stdint.h>

//Create 2 symbols formats for runtime - the emscripten(fastcomp) backend as well as the native llvm wasm backend
uint32_t (*Z_envZ___syscall140Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall140Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ___syscall146Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall146Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ___syscall54Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall54Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ___syscall6Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall6Z_iii)(uint32_t, uint32_t);

void initSyscalls()
{
	Z_envZ___syscall140Z_iii = Z_envZ____syscall140Z_iii = 0;
	Z_envZ___syscall146Z_iii = Z_envZ____syscall146Z_iii = 0;
	Z_envZ___syscall54Z_iii = Z_envZ____syscall54Z_iii = 0;
	Z_envZ___syscall6Z_iii = Z_envZ____syscall6Z_iii = 0;
}