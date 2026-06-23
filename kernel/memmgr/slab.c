#include "slab.h"
#include "buddy.h"

#define SLAB_SIZES 10
static uint64_t slab_size_table[SLAB_SIZES] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

// スラブヘッダ（各ページの先頭に置く）
typedef struct slab_header {
    struct slab_header* next;  // 次のスラブ
    uint64_t obj_size;         // オブジェクトサイズ
    uint64_t free_count;       // 空きオブジェクト数
    void*    free_list;        // 空きオブジェクトのリスト
} slab_header_t;

static slab_header_t* caches[SLAB_SIZES] = {0};

// サイズに対応するキャッシュインデックスを返す
static int size_to_index(uint64_t size) {
    for (int i = 0; i < SLAB_SIZES; i++)
        if (size <= slab_size_table[i]) return i;
    return -1;
}

// 新しいスラブページを確保して初期化
static slab_header_t* slab_new(uint64_t obj_size) {
    slab_header_t* slab = (slab_header_t*)buddy_alloc(0);
    if (!slab) return 0;

    slab->obj_size   = obj_size;
    slab->next       = 0;
    slab->free_list  = 0;

    // ヘッダの後ろにオブジェクトを並べる
    uint64_t offset = sizeof(slab_header_t);
    // アライメント調整
    offset = (offset + obj_size - 1) & ~(obj_size - 1);

    slab->free_count = 0;
    char* p = (char*)slab + offset;
    while (p + obj_size <= (char*)slab + PAGE_SIZE) {
        *(void**)p = slab->free_list;
        slab->free_list = p;
        slab->free_count++;
        p += obj_size;
    }
    return slab;
}

void slab_init() {
    for (int i = 0; i < SLAB_SIZES; i++)
        caches[i] = 0;
}

void* kmalloc(uint64_t size) {
    int idx = size_to_index(size);
    if (idx < 0) return 0;  // 4KB超はBuddy直接使う

    // 空きスラブを探す
    slab_header_t* slab = caches[idx];
    while (slab && slab->free_count == 0)
        slab = slab->next;

    // なければ新規作成
    if (!slab) {
        slab = slab_new(slab_size_table[idx]);
        if (!slab) return 0;
        slab->next  = caches[idx];
        caches[idx] = slab;
    }

    // オブジェクトを取り出す
    void* obj = slab->free_list;
    slab->free_list = *(void**)obj;
    slab->free_count--;
    return obj;
}

void kfree(void* ptr) {
    if (!ptr) return;

    // ページ先頭のスラブヘッダを取得
    slab_header_t* slab = (slab_header_t*)((uint64_t)ptr & ~(PAGE_SIZE - 1));

    *(void**)ptr = slab->free_list;
    slab->free_list = ptr;
    slab->free_count++;
}