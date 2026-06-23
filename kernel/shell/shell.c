#include "../io/vga.h"
#include "../io/keyboard.h"
#include "../kernelstd/string.h"
#include "../memmgr/buddy.h"
#include "../task/scheduler.h"
void shell_run() {
    vga_print("Here's a shell for CloverKernel!\n");
    vga_print("Copyright (C) 2026 CloverTech.\n");

    while(1) {
        vga_print("*");

        char input[256];
        int i = 0;

        // 入力受け取り
        while(1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                input[i] = '\0';
                vga_putchar('\n');
                break;
            } else if (c == '\b') {
                if (i > 0) {
                    i--;
                    vga_backspace();
                }
            } else {
                input[i++] = c;
                vga_putchar(c);
            }
        }

        // コマンド解析
        if (i == 0) continue;

        if (strcmp(input, "help") == 0) {
            vga_print("Commands: help, clear, meminfo, echo, tasks\n");
        } else if (strcmp(input, "clear") == 0) {
            vga_clear();
        } else if (strncmp(input, "echo ", 5) == 0) {
            vga_print(input + 5);
            vga_putchar('\n');
        } else if (strcmp(input, "meminfo") == 0) {
            vga_print("Total Memory: ");
            vga_print_dec(pmm_get_total_memory());
            vga_print(" bytes\n");
            vga_print("Free pages: ");
            vga_print_dec(buddy_free_pages());  // buddy.cに追加
            vga_print("\n");
        }else if (strcmp(input, "tasks") == 0) {
            vga_print("Task list:\n");
        int count = scheduler_get_task_count();
        vga_print("PID  NAME\n");
        for (int i = 0; i < count; i++) {
            task_t* t = scheduler_get_task(i);
            vga_print_dec(i);
            vga_print("    ");
            vga_print(t->name);
            vga_print("\n");
        }


        } else {
            vga_print("Unknown command: ");
            vga_print(input);
            vga_putchar('\n');
        }
    }
}