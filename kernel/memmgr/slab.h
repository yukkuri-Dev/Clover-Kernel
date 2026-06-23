#pragma once
#include "../kernelstd/types.h"

void slab_init();
void* kmalloc(uint64_t size);
void  kfree(void* ptr);
#define SLAB_SIZE 10