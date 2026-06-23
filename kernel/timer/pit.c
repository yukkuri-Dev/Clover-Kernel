#include "pit.h"
#include "../io/vga.h"
#include "../task/scheduler.h"
static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static uint64_t ticks = 0;

void pit_timer_handler() {
    ticks++;
    outb(0x20, 0x20);  // EOIを先に送る（schedule後に別タスクへ飛ぶと送られなくなる）
    schedule();
}

void pit_init() {
    outb(0x43, 0x36);
    outb(0x40, DIVISOR & 0xFF);
    outb(0x40, DIVISOR >> 8);
    
}

uint64_t pit_get_ticks() {
    return ticks;
}
