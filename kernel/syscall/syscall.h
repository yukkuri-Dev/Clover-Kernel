#pragma once
#include "../kernelstd/types.h"

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2

#define SYSCALL_MAX 3

typedef uint64_t (*syscall_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5);

void syscall_init();
