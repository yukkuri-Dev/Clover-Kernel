#include "vmm.h"
#include "buddy.h"
#include "../kernelstd/types.h"

// ブートローダーの恒等マッピング上では物理アドレス = 仮想アドレス
// buddy_allocが返すのは物理アドレスだが直接アクセス可能
static uint64_t* get_or_create(uint64_t* table, int index, uint64_t flags) {
    if (!(table[index] & PAGE_PRESENT)) {
        uint64_t page = (uint64_t)buddy_alloc(0);
        uint64_t* p = (uint64_t*)page;
        for (int i = 0; i < 512; i++) p[i] = 0;
        table[index] = page | flags;
    }
    return (uint64_t*)(table[index] & ~0xFFFULL);
}

// ブートローダーのPDPT/PDにPAGE_USERを付ける（初回のみ）
// ブートローダーが作ったPDPT(0x11000)/PD(0x12000)はUSERフラグなしのため
// Ring3がカーネルコードにアクセスできるよう一括でUSERを立てる
void vmm_init() {
    static int patched = 0;
    if (patched) return;
    patched = 1;

    uint64_t* pdpt = (uint64_t*)0x11000;
    for (int i = 0; i < 512; i++) {
        if (pdpt[i] & PAGE_PRESENT)
            pdpt[i] |= PAGE_USER;
    }

    uint64_t* pd = (uint64_t*)0x12000;
    for (int i = 0; i < 512; i++) {
        if (pd[i] & PAGE_PRESENT)
            pd[i] |= PAGE_USER;
    }

    // CR3を再ロードしてTLBをフラッシュ
    __asm__ volatile (
        "mov %%cr3, %%rax\n"
        "mov %%rax, %%cr3\n"
        : : : "rax", "memory"
    );
}

extern void vga_print(const char*);
extern void vga_print_hex(uint64_t);

page_table_t vmm_create_address_space() {
    vmm_init();

    uint64_t phys = (uint64_t)buddy_alloc(0);
    uint64_t* pml4 = (uint64_t*)phys;
    for (int i = 0; i < 512; i++) pml4[i] = 0;

    // カーネルPML4をコピー（PML4にもUSERを付ける）
    uint64_t* kpml4 = (uint64_t*)0x10000;
    for (int i = 0; i < 512; i++) {
        if (kpml4[i] & PAGE_PRESENT)
            pml4[i] = kpml4[i] | PAGE_USER;
        else
            pml4[i] = 0;
    }

    return (page_table_t)phys;
}

void vmm_map_page(page_table_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t* pml4 = (uint64_t*)(uint64_t)pml4_phys;

    int pml4_idx = (virt >> 39) & 0x1FF;
    int pdpt_idx = (virt >> 30) & 0x1FF;
    int pd_idx   = (virt >> 21) & 0x1FF;
    int pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t* pdpt = get_or_create(pml4, pml4_idx, PAGE_PRESENT | PAGE_WRITE | flags);
    uint64_t* pd   = get_or_create(pdpt, pdpt_idx, PAGE_PRESENT | PAGE_WRITE | flags);
    uint64_t* pt   = get_or_create(pd,   pd_idx,   PAGE_PRESENT | PAGE_WRITE | flags);

    pt[pt_idx] = phys | flags;
}

void vmm_switch(page_table_t pml4_phys) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint64_t)pml4_phys) : "memory");
}