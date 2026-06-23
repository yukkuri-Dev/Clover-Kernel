#include "pic.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void pic_init() {
    // ICW1: 初期化開始
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    // ICW2: 割り込みベクタのオフセット
    outb(PIC1_DATA, 0x20);  // IRQ0-7 → vector 32-39
    outb(PIC2_DATA, 0x28);  // IRQ8-15 → vector 40-47
    // ICW3: カスケード設定
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    // ICW4: 8086モード
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    // 全IRQをマスク
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    // IRQ1 (キーボード) マスク解除
    outb(PIC1_DATA, 0xFD);  // 11111101 = IRQ1だけ解除
    outb(PIC2_DATA, 0xFF);  // 全部マスク
    // IRQ0 (PIT) マスク解除
    outb(PIC1_DATA, 0xFC);  // IRQ0とIRQ1だけ有効
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}