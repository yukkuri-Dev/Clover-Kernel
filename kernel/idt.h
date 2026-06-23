#pragma once 
#include "kernelstd/types.h"

typedef struct {
    uint16_t offset_low;   // 0-15ビット
    uint16_t selector;     // セレクタ
    uint8_t  ist;          // ISTインデックス
    uint8_t  type_attr;    // タイプと属性
    uint16_t offset_mid;   // 16-31ビット
    uint32_t offset_high;  // 32-63ビット
    uint32_t zero;         // 予約領域
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;        // IDTのサイズ
    uint64_t base;         // IDTのベースアドレス
} __attribute__((packed)) idt_ptr_t;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
void idt_init();