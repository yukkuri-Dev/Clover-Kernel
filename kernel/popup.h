#include "io/vga.h"
void its_OK(){
    vga_print("[ ");
    vga_color_print("OK", 0x0A);
    vga_print(" ]");
}
void its_NG(){
    vga_print("[ ");
    vga_color_print("NG", 0x0C);
    vga_print(" ]");
}
void its_WARN(){
    vga_print("[");
    vga_color_print("WARN", 0x0E);
    vga_print("]");
}
void its_INFO(){
    vga_print("[");
    vga_color_print("INFO", 0x09);
    vga_print("]");
}
void its_DEBUG(){
    vga_print("[");
    vga_color_print("DEBG", 0x0D);
    vga_print("]");
}
void its_LOAD(){
    vga_print("[");
    vga_color_print("LOAD", 0x0B);
    vga_print("]");
}
void its_FAIL(){
    vga_print("[");
    vga_color_print("FAIL", 0x0C);
    vga_print("]");
}
void its_TASK(){
    vga_print("[");
    vga_color_print("TASK", 0x0B);
    vga_print("]");
}