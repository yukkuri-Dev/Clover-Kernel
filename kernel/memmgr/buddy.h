#pragma once
#include "../kernelstd/types.h"

#define PAGE_SIZE 4096
#define MAX_ORDER 9  // 最大2^9 * 4KB = 2MB

void buddy_init(uint64_t base, uint64_t length);
void* buddy_alloc(int order);
void buddy_free(void* ptr, int order);