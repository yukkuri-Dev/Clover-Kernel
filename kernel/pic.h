#pragma once
#include "kernelstd/types.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

void pic_init();
void pic_send_eoi(uint8_t irq);