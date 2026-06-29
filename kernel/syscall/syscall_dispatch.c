#include "syscall.h"
#include "../io/vga.h"
#include "../task/scheduler.h"

static uint64_t sys_exit(uint64_t code,
                         uint64_t a2 __attribute__((unused)),
                         uint64_t a3 __attribute__((unused)),
                         uint64_t a4 __attribute__((unused)),
                         uint64_t a5 __attribute__((unused)),
                         uint64_t a6 __attribute__((unused))) {
    (void)code;
    task_exit();
    return 0;
}

static uint64_t sys_write(uint64_t fd,
                          uint64_t buf,
                          uint64_t len,
                          uint64_t a4 __attribute__((unused)),
                          uint64_t a5 __attribute__((unused)),
                          uint64_t a6 __attribute__((unused))) {
    if (fd != 1 && fd != 2) return (uint64_t)-1;
    const char* s = (const char*)buf;
    for (uint64_t i = 0; i < len; i++)
        vga_putchar(s[i]);
    return len;
}

static uint64_t sys_read(uint64_t a1 __attribute__((unused)),
                         uint64_t a2 __attribute__((unused)),
                         uint64_t a3 __attribute__((unused)),
                         uint64_t a4 __attribute__((unused)),
                         uint64_t a5 __attribute__((unused)),
                         uint64_t a6 __attribute__((unused))) {
    return (uint64_t)-1;
}

static syscall_fn syscall_table[SYSCALL_MAX] = {
    sys_exit,
    sys_write,
    sys_read,
};

uint64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5) {
    if (num >= SYSCALL_MAX) return (uint64_t)-1;
    return syscall_table[num](a1, a2, a3, a4, a5, 0);
}

void syscall_init() {
    // EFER.SCE (bit0) を有効化してSYSCALL命令を使えるようにする
    __asm__ volatile (
        "mov $0xC0000080, %%ecx\n"  // EFER MSR
        "rdmsr\n"
        "or $1, %%eax\n"            // SCEビットをセット
        "wrmsr\n"
        : : : "eax", "ecx", "edx"
    );

    // STAR MSR:
    //   bits[47:32] = SYSCALL用CS  → KCode = 0x08 (SYSCALL: CS=0x08, SS=0x10)
    //   bits[63:48] = SYSRET用ベース。sysretqはCS=base+16|RPL3, SS=base+8|RPL3
    //   GDT: 0x08=KCode, 0x10=KData, 0x18=UData, 0x20=UCode
    //   base=0x0010 → CS=0x20|3=0x23(UCode ✓), SS=0x18|3=0x1B(UData ✓)
    uint64_t star = ((uint64_t)0x0008 << 32) | ((uint64_t)0x0010 << 48);
    __asm__ volatile (
        "mov $0xC0000081, %%ecx\n"
        "mov %%eax, %%eax\n"
        "mov %%edx, %%edx\n"
        "wrmsr\n"
        : : "A"(star) : "ecx"
    );

    // LSTAR MSR: syscall_entryのアドレスを設定
    extern void syscall_entry();
    uint64_t entry = (uint64_t)syscall_entry;
    __asm__ volatile (
        "mov $0xC0000082, %%ecx\n"
        "wrmsr\n"
        : : "A"(entry) : "ecx"
    );

    // SFMASK MSR: SYSCALL時にマスクするrflagsビット (IFビット=0x200をマスク)
    __asm__ volatile (
        "mov $0xC0000084, %%ecx\n"
        "mov $0x200, %%eax\n"
        "xor %%edx, %%edx\n"
        "wrmsr\n"
        : : : "eax", "ecx", "edx"
    );
}
