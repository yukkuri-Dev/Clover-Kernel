#include "pmm.h"
#include "buddy.h"
#include "../io/vga.h"

static e820_entry_t* memmap;
static uint64_t total_memory = 0;

void pmm_init() {
    uint64_t addr = 0x500;
    memmap = (e820_entry_t*)addr;
    for (int i = 0; i < 32; i++) {
        if (memmap[i].length == 0) break;
        if (memmap[i].type == E820_USABLE && memmap[i].base >= 0x100000) {
            total_memory += memmap[i].length;
            if (memmap[i].base >= 0x100000) {
                uint64_t safe_len = memmap[i].length;
                if (safe_len > 0x300000) safe_len = 0x300000;  // 3MBに限定


                //debug
                vga_print("buddy_init:");
                vga_print_hex(memmap[i].base);
                vga_print("\n");

                buddy_init(memmap[i].base, safe_len);
            }
        }
    }
}
uint64_t pmm_get_total_memory() {
    return total_memory;
}