// kernel/panic.c
#include "panic.h"
#include "io/vga.h"

void kernel_panic(const char* msg,int color) {
    __asm__ volatile ("cli");  // 割り込み無効化
    vga_color_fill(color);  // 指定された色で背景を塗りつぶす
    vga_print("!!![KERNEL PANIC]!!!\n");
    vga_print(msg);
    vga_print("\nSystem halted.");

    while(1) __asm__ volatile ("hlt");
}