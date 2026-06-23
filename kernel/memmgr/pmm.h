#pragma once
#include "../kernelstd/types.h"

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

#define E820_USABLE 1

void pmm_init();
uint64_t pmm_get_total_memory();