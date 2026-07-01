#pragma once
#include "../kernelstd/types.h"
#include "../syscall/syscall.h"

// Ring3から呼ぶsyscallラッパー。
// 必ずインライン展開させ、ユーザー関数を単一・位置独立な塊にする
// （別関数へのcallがあるとそのhigh halfアドレスを実行してしまうため）。
__attribute__((always_inline))
static inline uint64_t syscall(uint64_t num,
                               uint64_t a1, uint64_t a2, uint64_t a3,
                               uint64_t a4, uint64_t a5) {
    uint64_t ret;
    register uint64_t r10 __asm__("r10") = a4;
    register uint64_t r8  __asm__("r8")  = a5;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "0"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return ret;
}

__attribute__((always_inline))
static inline void user_write(const char* s, uint64_t len) {
    syscall(SYS_WRITE, 1, (uint64_t)s, len, 0, 0);
}

__attribute__((always_inline))
static inline void user_exit(uint64_t code) {
    syscall(SYS_EXIT, code, 0, 0, 0, 0);
}
