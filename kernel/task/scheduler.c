#include "scheduler.h"
#include "../memmgr/slab.h"
#include "../memmgr/buddy.h"
#include "../memmgr/vmm.h"

#define MAX_TASKS 16
extern void context_switch(task_t* current, task_t* next);
static task_t* current_task = 0;
static task_t* task_list[MAX_TASKS] = {0};
static int task_count = 0;

void scheduler_init() {
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->state      = 1;
    kernel_task->next       = 0;
    kernel_task->page_table = 0;  // カーネルは現在のCR3をそのまま使う
    current_task = kernel_task;
    task_list[task_count++] = kernel_task;
}

void scheduler_add_task(void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task  = (task_t*)kmalloc(sizeof(task_t));
    void*   stack = buddy_alloc(0);  // 4KBスタック

    // タスク用アドレス空間を作成（カーネルマッピングを引き継ぐ）
    task->page_table = vmm_create_address_space();

    // iretq フレーム: [rip, cs, rflags, rsp, ss]
    uint64_t* sp = (uint64_t*)((uint64_t)stack + PAGE_SIZE);
    uint64_t initial_rsp = (uint64_t)sp;
    *--sp = 0x10;            // ss
    *--sp = initial_rsp;     // rsp
    *--sp = 0x202;           // rflags: IF=1
    *--sp = 0x18;            // cs
    *--sp = (uint64_t)entry; // rip

    task->rsp   = (uint64_t)sp;
    task->stack = stack;
    task->state = 1;
    task->next  = 0;

    task_list[task_count++] = task;
}

void schedule() {
    if (task_count < 2) return;

    // ラウンドロビン
    static int current_idx = 0;
    int next_idx = (current_idx + 1) % task_count;

    task_t* prev = task_list[current_idx];
    task_t* next = task_list[next_idx];
    current_idx  = next_idx;

    if (next->page_table) {
        // タスクのページテーブルに切り替え
        vmm_switch(next->page_table);
    }

    context_switch(prev, next);
}
