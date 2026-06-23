#include "buddy.h"
#include "../io/vga.h"
#define MAX_PAGES 131072  // 512MB / 4KB

// フリーリストの先頭
static uint64_t free_list[MAX_ORDER + 1];
static uint64_t buddy_base;
static uint64_t buddy_pages;

// ページのフラグ（使用中/空き）
static uint8_t page_flags[MAX_PAGES];

void buddy_init(uint64_t base, uint64_t length) {

    buddy_base  = base;
    buddy_pages = length / PAGE_SIZE;
    //debug
    vga_print("buddy_pages: ");
    vga_print_dec(buddy_pages);
    vga_print("\n");
    vga_print("MAX_ORDER: ");
    vga_print_dec(1 << MAX_ORDER);
    vga_print("\n");

    // フリーリスト初期化
    for (int i = 0; i <= MAX_ORDER; i++)
        free_list[i] = 0;

    // 全ページを最大orderのブロックとして登録
    uint64_t i = 0;
    int order = MAX_ORDER;
    while (i + (1 << order) <= buddy_pages) {
        buddy_free((void*)(base + i * PAGE_SIZE), order);
        i += (1 << order);
    }
    //debug
    vga_print("free_list[9]: ");
    vga_print_hex(free_list[MAX_ORDER]);
    vga_print("\n");
}

void* buddy_alloc(int order) {
    // 要求orderが不正な場合は失敗
    if (order < 0 || order > MAX_ORDER) return 0;
    // 要求orderから上を探す
    for (int o = order; o <= MAX_ORDER; o++) {
        if (free_list[o] == 0) continue;

        // ブロックを取り出す
        uint64_t addr = free_list[o];
        free_list[o] = *(uint64_t*)addr;

        // 余分なブロックを分割して戻す
        while (o > order) {
            o--;
            uint64_t buddy_addr = addr + (uint64_t)(1 << o) * PAGE_SIZE;
            *(uint64_t*)buddy_addr = free_list[o];
            free_list[o] = buddy_addr;
        }
        return (void*)addr;
    }
    return 0;  // メモリ不足
}

void buddy_free(void* ptr, int order) {
    uint64_t addr = (uint64_t)ptr;

    // バディのアドレスを計算してマージ
    while (order < MAX_ORDER) {
        uint64_t offset = addr - buddy_base;
        uint64_t buddy_offset = offset ^ ((uint64_t)(1 << order) * PAGE_SIZE);
        uint64_t buddy_addr = buddy_base + buddy_offset;

        // バディがフリーリストにあるか探す
        uint64_t* p = &free_list[order];
        while (*p && *p != buddy_addr)
            p = (uint64_t*)*p;

        if (*p == 0) break;  // バディが使用中

        // バディをフリーリストから外してマージ
        *p = *(uint64_t*)buddy_addr;
        addr = addr < buddy_addr ? addr : buddy_addr;
        order++;
    }

    *(uint64_t*)addr = free_list[order];
    free_list[order] = addr;
}

uint64_t buddy_free_pages() {
    uint64_t count = 0;
    for (int o = 0; o <= MAX_ORDER; o++) {
        uint64_t p = free_list[o];
        while (p) {
            count += (1 << o);
            p = *(uint64_t*)p;
        }
    }
    return count;
}