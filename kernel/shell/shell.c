#include "../io/vga.h"
#include "../io/keyboard.h"
#include "../kernelstd/string.h"
#include "../memmgr/buddy.h"
#include "../task/scheduler.h"

void Hello(){
    vga_print("Blank task is running...\n");
}
extern int nope = 0;

void license() {
    vga_print("Clover Kernel's shell v0.1.0\n");
    vga_print("Kernel Built-in\n");
    vga_print("Copyright (c) 2026 Clover Tech\n");
    vga_print("This software is released under the MIT License.\n");
    vga_print("https://opensource.org/licenses/MIT\n");
    vga_print("\n");
}
void shell_run() {
    license();

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


            vga_print("Commands: help, clear, meminfo, echo, tasks, crash\n");
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
            vga_print("PID  TASK_ADDR        NAME_ADDR        NAME\n");
            for (int i = 0; i < count; i++) {
                task_t* t = scheduler_get_task(i);
                vga_print_dec(i);
                vga_print("    ");
                vga_print_hex((uint64_t)t);
                vga_print("  ");
                vga_print_hex((uint64_t)t->name);
                vga_print("  [");
                vga_print(t->name);
                vga_print("]\n");
            }
        } else if (strcmp(input, "crash") == 0) {
            if (nope < 1) {
                vga_print("if you continue to type 'crash', the system will crash.\n");
                vga_print("\n");
                nope++;
                continue;
            }else {
                vga_print("Triggering a crash...\n");
                __asm__ volatile ("int3");  // デバッグ割り込みを発生させる
            }


        } else if (strcmp(input, "ver") == 0) {
            license();


        }else if (strcmp(input,"whoami") == 0){
            vga_print("kernel(PID: 0)\n");


        }else if (strcmp(input,"yes") == 0){
            vga_print("yes\n");

        } else {
            vga_print("Unknown command: ");
            vga_print(input);
            vga_putchar('\n');
        }
    }
}