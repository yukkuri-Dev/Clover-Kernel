#include "scheduler.h"
#include "../memmgr/slab.h"
#include "../memmgr/buddy.h"

#define MAX_TASKS 16
extern void context_switch(task_t* current, task_t* next);
static task_t* current_task = 0;
static task_t* task_list[MAX_TASKS] = {0};
static int task_count = 0;

void scheduler_init() {
    // カーネル自身をtask[0]として登録
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->state = 1;  // RUNNING
    kernel_task->next  = 0;
    current_task = kernel_task;
    task_list[task_count++] = kernel_task;
}

void scheduler_add_task(void (*entry)(), uint64_t stack_size) {
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    void*   stack = buddy_alloc(0);  // 4KBスタック

    // iretq フレームを積む: [rip, cs, rflags, rsp, ss] (rip が最上位)
    uint64_t* sp = (uint64_t*)((uint64_t)stack + stack_size);
    uint64_t initial_rsp = (uint64_t)sp;  // タスク起動時の rsp (ss push 前)
    *--sp = 0x10;           // ss
    *--sp = initial_rsp;    // rsp
    *--sp = 0x202;          // rflags: IF=1, reserved bit
    *--sp = 0x18;           // cs
    *--sp = (uint64_t)entry; // rip

    task->rsp   = (uint64_t)sp;
    task->stack = stack;
    task->state = 1;  // READY
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

    context_switch(prev, next);
}
