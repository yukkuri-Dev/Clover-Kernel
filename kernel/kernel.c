#include "idt.h"
#include "pic.h"
#include "io/vga.h"
#include "memmgr/pmm.h"
#include "memmgr/buddy.h"
#include "popup.h"
#include "timer/pit.h"
#include "task/scheduler.h"
#include "memmgr/slab.h"
int kernel_debug = 1;
void logo(){
    if (kernel_debug == 1) {
        vga_print("  Clover Kernel alpha 0.1.0-Debug-enable\n");
    }else{
        vga_print("     ______   __  Kernel alpha 0.1.0\n");
        vga_print("    /  ___/\\ / /\\   ______    __   __    _______     ______ \n");
        vga_print("   /  /\\__\\// / /  / ___  /\\ I  I / /\\  /  __  /\\   /  ___/\\ \n");
        vga_print("  /  /_/   / /_/  / /__/ / / I  I/ / / /  ____/ /  /  /\\__\\/       \n");
        vga_print(" /_____/\\ / ___/\\/______/ /  I___ / / /______/\\/  /__/ /           \n");
        vga_print(" \\_____\\/ \\____\\/\\______\\/    \\___\\/  \\______\\/   \\__\\/            \n");   
    }
}

void task_a() {
    vga_print("Task A is running...\n");
    char val = 'A';
    while(1) {
        vga_print("A:");
        vga_putchar(val);
        vga_print(" ");
        for(volatile int i = 0; i < 1000000; i++);
    }
}

void task_b() {
    vga_print("Task B is running...\n");
    char val = 'B';
    while(1) {
        vga_print("B:");
        vga_putchar(val);
        vga_print(" ");
        for(volatile int i = 0; i < 1000000; i++);
    }
}
void task_c() {
    //__asm__ volatile ("int3");// デバッグ割り込み(KernelPanic)を発生させる
    while(1) {
        
        for(volatile int i = 0; i < 1000000; i++){

            //vga_print_hex(i);
        }
    }
}



void kernel_main(){
    int kernel_debug = 1;// デバッグ用フラグ
    //みてわかるだろ？？
    // 割り込みのしょきかだよこれ
    idt_init();
    pic_init();
    //ここまでで完了
    vga_init();
    vga_clear();
    vga_print("PML4[256]: ");
    vga_print_hex(*(uint64_t*)(0x10000 + 256*8));
    vga_print("\n");
    its_OK();vga_print(" Kernel is loaded!\n");
    pmm_init();
    
    its_OK();vga_print(" Physical Memory Manager is initialized!\n\n");
    vga_print("Total Memory: ");vga_print_dec(pmm_get_total_memory());vga_print(" bytes may be available\n");
    // ここまででメモリマネージャの初期化が完了
    logo();
    vga_print("Welcome to Clover's Kernel! :-)\n");
    void* p = buddy_alloc(-1);  // 1ページ (4KB)
    its_INFO();vga_print("illegal order requested\n");

    if ((uint64_t)p == 0) {
        its_NG();
        vga_print(" Failed to allocate memory\n");
    } else {
        its_OK();
        vga_print(" Alloc: ");
        vga_print_hex((uint64_t)p);
    }
    void* q = buddy_alloc(0);  // 1ページ (4KB)
    its_INFO();vga_print("legal order requested\n");
    if ((uint64_t)q == 0) {
        its_NG();
        vga_print(" Failed to allocate memory\n");
    } else {
        its_OK();
        vga_print(" Alloc: ");
        vga_print_hex((uint64_t)q);
    }
    // kernel_main に追加
vga_print("\n");

slab_init();
its_OK();vga_print(" Slab Allocator initialized!\n");
// 小さいサイズを確保
void* a = kmalloc(16);
void* b = kmalloc(64);
void* c = kmalloc(256);



if (kernel_debug != 1){
vga_print("kmalloc(16):  ");
vga_print_hex((uint64_t)a);
vga_print("\n");
vga_print("kmalloc(64):  ");
vga_print_hex((uint64_t)b);
vga_print("\n");
vga_print("kmalloc(256): ");
vga_print_hex((uint64_t)c);
vga_print("\n");

}
// freeして再確保したら同じアドレスが返ってくるはず
kfree(a);
void* d = kmalloc(16);
vga_print("realloc(16):  ");
vga_print_hex((uint64_t)d);
vga_print("\n");
// a == d なら再利用できてる
    vga_print("\n");
    vga_print("Testing scheduler...\n");
    scheduler_init();
    vga_print("Scheduler initialized!\n");
    scheduler_add_task(task_c, 4096);
    vga_print("Added Task C\n");
    scheduler_add_task(task_b, 4096);
    vga_print("Added Task B\n");
    scheduler_add_task(task_a, 4096);
    vga_print("Added Task A\n");

    pit_init();
    __asm__ volatile ("sti");

    while(1) {
        __asm__ volatile ("hlt");
    }
}


