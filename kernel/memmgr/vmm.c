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

page_table_t vmm_create_address_space() {
    uint64_t phys = (uint64_t)buddy_alloc(0);
    uint64_t* pml4 = (uint64_t*)phys;
    for (int i = 0; i < 512; i++) pml4[i] = 0;

    // カーネルPML4の全エントリをコピー
    // PML4[0]   : 恒等マッピング（カーネルコード・スタック・物理メモリ領域）
    // PML4[256+]: higher half（将来のカーネル仮想空間用）
    uint64_t* kpml4 = (uint64_t*)0x10000;
    for (int i = 0; i < 512; i++)
        pml4[i] = kpml4[i];

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