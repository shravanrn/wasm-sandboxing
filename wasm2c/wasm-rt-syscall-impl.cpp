#include <stdint.h>

uint32_t (*Z_envZ____syscall140Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall146Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall54Z_iii)(uint32_t, uint32_t);
uint32_t (*Z_envZ____syscall6Z_iii)(uint32_t, uint32_t);

void initSyscalls()
{
	Z_envZ____syscall140Z_iii = 0;
	Z_envZ____syscall146Z_iii = 0;
	Z_envZ____syscall54Z_iii = 0;
	Z_envZ____syscall6Z_iii = 0;
}