#include "vga.h"

static int cursor_x = 0;
static int cursor_y = 0;
static char* vga = (char*)0xB8000;

void vga_clear() {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i]   = ' ';
        vga[i+1] = 0x0F;
    }
    cursor_x = 0;
    cursor_y = 0;
}
void vga_color_fill(char color) {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i]   = ' ';
        vga[i+1] = color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_init() {
    vga_clear();
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    int offset = (cursor_y * 80 + cursor_x) * 2;
    vga[offset]   = c;
    vga[offset+1] = 0x0F;
    cursor_x++;
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= 25) {
        vga_scroll();
    }
}

void vga_putchar_color(char c, char color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    int offset = (cursor_y * 80 + cursor_x) * 2;
    vga[offset]   = c;
    vga[offset+1] = color;
    cursor_x++;
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= 25) {
        vga_scroll();
    }
}
void vga_color_print(const char* str, char color) {
    while (*str) vga_putchar_color(*str++, color);
}
void vga_print(const char* str) {
    while (*str) vga_putchar(*str++);
}

void vga_print_hex(uint64_t val) {
    char buf[17];
    buf[16] = 0;
    for (int i = 15; i >= 0; i--) {
        int d = val & 0xF;
        buf[i] = d < 10 ? '0' + d : 'A' + d - 10;
        val >>= 4;
    }
    vga_print("0x");
    vga_print(buf);
}

void vga_print_dec(uint64_t val) {
    if (val == 0) { vga_putchar('0'); return; }
    char buf[21];
    int i = 20;
    buf[i] = 0;
    while (val > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }
    vga_print(&buf[i]);
}
void vga_scroll() {
    // 1行上にコピー
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 80; x++) {
            int dst = (y * 80 + x) * 2;
            int src = ((y+1) * 80 + x) * 2;
            vga[dst]   = vga[src];
            vga[dst+1] = vga[src+1];
        }
    }
    // 最終行をクリア
    for (int x = 0; x < 80; x++) {
        int offset = (24 * 80 + x) * 2;
        vga[offset]   = ' ';
        vga[offset+1] = 0x0F;
    }
    cursor_y = 24;
}