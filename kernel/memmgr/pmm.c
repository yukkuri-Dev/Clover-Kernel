#include "pmm.h"
#include "buddy.h"
#include "../io/vga.h"

static e820_entry_t* memmap;
static uint64_t total_memory = 0;

void pmm_init() {
    uint64_t addr = 0x500;
    memmap = (e820_entry_t*)addr;

    // 一番大きいusableエントリをbuddy_initに渡す
    uint64_t best_base = 0, best_len = 0;
    for (int i = 0; i < 32; i++) {
        if (memmap[i].length == 0) break;
        if (memmap[i].type == E820_USABLE && memmap[i].base >= 0x100000) {
            total_memory += memmap[i].length;
            if (memmap[i].length > best_len) {
                best_len  = memmap[i].length;
                best_base = memmap[i].base;
            }
        }
    }
    if (best_len > 0)
        buddy_init(best_base, best_len);
}
uint64_t pmm_get_total_memory() {
    return total_memory;
}