#pragma once
#include "../kernelstd/types.h"

#define PAGE_PRESENT  0x01
#define PAGE_WRITE    0x02
#define PAGE_USER     0x04  // Ring3からアクセス可能

typedef uint64_t* page_table_t;

void vmm_init();
page_table_t vmm_create_address_space();
void vmm_map_page(page_table_t pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_switch(page_table_t pml4);

