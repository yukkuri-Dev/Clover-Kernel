#include "keyboard.h"

static char scancode_map[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' '
};

#define BUFFER_SIZE 256
static char buffer[BUFFER_SIZE];
static int buf_head = 0;
static int buf_tail = 0;

void keyboard_handler_c() {
    uint8_t sc;
    __asm__ volatile ("inb $0x60, %0" : "=a"(sc));
    
    if (sc & 0x80) return;  // キーアップは無視
    
    if (sc < sizeof(scancode_map)) {
        char c = scancode_map[sc];
        if (c) {
            buffer[buf_head % BUFFER_SIZE] = c;
            buf_head++;
        }
    }
}

char keyboard_getchar() {
    while (buf_head == buf_tail)
        __asm__ volatile ("hlt");  // 入力待ち
    char c = buffer[buf_tail % BUFFER_SIZE];
    buf_tail++;
    return c;
}

void keyboard_init() {
    // PICのIRQ1マスクを解除はpic.cで済んでるはず
}