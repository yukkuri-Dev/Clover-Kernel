// vga.h
#pragma once
#include "../kernelstd/types.h"
/** 
 * @brief VGAテキストモードの初期化と文字出力関数
 */
void vga_init();
void vga_putchar(char c);
void vga_print(const char* str);
void vga_print_hex(uint64_t val);  // 数値表示
void vga_print_dec(uint64_t val);
void vga_newline();
void vga_clear();
void vga_putchar_color(char c, char color) ;
void vga_color_print(const char* str, char color);
void vga_color_fill(char color) ;
void vga_backspace();
void vga_set_cursor(int x, int y);