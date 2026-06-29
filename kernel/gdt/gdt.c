#include "gdt.h"
#include "../io/vga.h"

// GDTをバイト配列で管理してレイアウトを確定させる
// オフセット: 0x00=Null, 0x08=KCode, 0x10=KData, 0x18=UCode, 0x20=UData, 0x28=TSS(16B)
#define GDT_SIZE (5 * 8 + 16)  // 5エントリ×8B + TSSディスクリプタ16B = 56B

static uint8_t gdt[GDT_SIZE];
static gdt_descriptor_t gdtr;
static tss_t tss;

static void set_entry(int offset, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t gran) {
    gdt[offset + 0] = limit & 0xFF;
    gdt[offset + 1] = (limit >> 8) & 0xFF;
    gdt[offset + 2] = base & 0xFF;
    gdt[offset + 3] = (base >> 8) & 0xFF;
    gdt[offset + 4] = (base >> 16) & 0xFF;
    gdt[offset + 5] = access;
    gdt[offset + 6] = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[offset + 7] = (base >> 24) & 0xFF;
}

static void set_tss_entry(int offset, uint64_t base, uint32_t limit) {
    gdt[offset +  0] = limit & 0xFF;
    gdt[offset +  1] = (limit >> 8) & 0xFF;
    gdt[offset +  2] = base & 0xFF;
    gdt[offset +  3] = (base >> 8) & 0xFF;
    gdt[offset +  4] = (base >> 16) & 0xFF;
    gdt[offset +  5] = 0x89;  // Present, Ring0, 64bit TSS Available
    gdt[offset +  6] = (limit >> 16) & 0x0F;
    gdt[offset +  7] = (base >> 24) & 0xFF;
    gdt[offset +  8] = (base >> 32) & 0xFF;
    gdt[offset +  9] = (base >> 40) & 0xFF;
    gdt[offset + 10] = (base >> 48) & 0xFF;
    gdt[offset + 11] = (base >> 56) & 0xFF;
    gdt[offset + 12] = 0;
    gdt[offset + 13] = 0;
    gdt[offset + 14] = 0;
    gdt[offset + 15] = 0;
}

void gdt_init() {
    for (int i = 0; i < GDT_SIZE; i++) gdt[i] = 0;

    set_entry(0x00, 0, 0,       0,    0   );  // Null
    set_entry(0x08, 0, 0xFFFFF, 0x9A, 0xA0);  // Kernel Code (64bit, Ring0)
    set_entry(0x10, 0, 0xFFFFF, 0x92, 0xC0);  // Kernel Data (Ring0)
    // SYSRET の都合で UData(0x18) を UCode(0x20) より前に置く:
    //   STAR[63:48]=0x10 → SYSRET CS=0x10+16=0x20|3=0x23(UCode),
    //                              SS=0x10+8 =0x18|3=0x1B(UData)
    set_entry(0x18, 0, 0xFFFFF, 0xF2, 0xC0);  // User Data   (Ring3, sel=0x1B)
    set_entry(0x20, 0, 0xFFFFF, 0xFA, 0xA0);  // User Code   (64bit, Ring3, sel=0x23)

    // TSS初期化
    for (int i = 0; i < (int)sizeof(tss_t); i++) ((uint8_t*)&tss)[i] = 0;
    tss.iopb_offset = sizeof(tss_t);
    set_tss_entry(0x28, (uint64_t)&tss, sizeof(tss_t) - 1);

    gdtr.limit = GDT_SIZE - 1;
    gdtr.base  = (uint64_t)gdt;

    __asm__ volatile ("lgdt %0" : : "m"(gdtr) : "memory");

    // CS更新（far return trick）
    __asm__ volatile (
        "lea 1f(%%rip), %%rax\n"
        "push $0x08\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        : : : "rax", "memory"
    );

    // データセグメント更新
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "ax"
    );

    // TSSロード
    __asm__ volatile ("ltr %0" : : "r"((uint16_t)0x28));

    vga_print("GDT & TSS loaded\n");
}

void tss_set_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
