#include "idt.h"
#include "panic.h"

extern void gp_fault_handler();
extern void pf_handler();
extern void df_handler();
extern void keyboard_handler();
extern void pit_handler();
static idt_entry_t idt[256];
static idt_ptr_t idtr;

void idt_set_entry(int vector, void* handler, uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x18;  // 64bitコードセグメント
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void idt_init() {
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)idt;

    idt_set_entry(13, gp_fault_handler, 0x8E);  // #GP
    idt_set_entry(14, pf_handler, 0x8E);         // #PF
    idt_set_entry(33, keyboard_handler, 0x8E);  // IRQ1 (キーボード)
    idt_set_entry(32, pit_handler, 0x8E);       //  IRQ0 (PIT)
    idt_set_entry(8, df_handler, 0x8E);          // #DF
    // IDTRにロード
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}


void gp_fault_handler_c() {
    kernel_panic("General Protection Fault", 0xCC);
}

void pf_handler_c() {
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    vga_print("\nPage Fault at: ");
    vga_print_hex(cr2);
    kernel_panic("Page Fault", 0xCC);
}

void df_handler_c() {
    kernel_panic("Double Fault - System Halted", 0xBC);
}