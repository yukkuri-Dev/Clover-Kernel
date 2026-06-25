#include "scheduler.h"
#include "../memmgr/slab.h"
#include "../memmgr/buddy.h"
#include "../memmgr/vmm.h"
#include "../gdt/gdt.h"

#define MAX_TASKS 32
extern void context_switch(task_t* current, task_t* next);
static task_t* current_task = 0;
static task_t* task_list[MAX_TASKS] = {0};
static int task_count = 0;

void scheduler_init() {
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->state      = 1;
    kernel_task->next       = 0;
    kernel_task->page_table = 0;
    kernel_task->kstack_top = 0;
    // kernel_taskに名前を設定
    kernel_task->name[0] = 'k';
    kernel_task->name[1] = 'e';
    kernel_task->name[2] = 'r';
    kernel_task->name[3] = 'n';
    kernel_task->name[4] = 'e';
    kernel_task->name[5] = 'l';
    kernel_task->name[6] = '\0';
    current_task = kernel_task;
    task_list[task_count++] = kernel_task;
}

void scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    void*   stack = buddy_alloc(0);  // 4KBスタック

    // タスク用アドレス空間を作成（カーネルマッピングを引き継ぐ）
    task->page_table = vmm_create_address_space();

    // iretq フレーム: [rip, cs, rflags, rsp, ss]
    uint64_t* sp = (uint64_t*)((uint64_t)stack + PAGE_SIZE);
    uint64_t initial_rsp = (uint64_t)sp;
    *--sp = 0x10;            // ss = Kernel Data
    *--sp = initial_rsp;     // rsp
    *--sp = 0x202;           // rflags: IF=1
    *--sp = 0x08;            // cs = Kernel Code
    *--sp = (uint64_t)entry; // rip

    task->rsp        = (uint64_t)sp;
    task->kstack_top = 0;
    task->stack      = stack;
    task->state      = 1;
    task->next       = 0;

    int i = 0;
    while (name[i] && i < 31) {
        task->name[i] = name[i];
        i++;
    }
    task->name[i] = '\0';

    task_list[task_count++] = task;




}

// Ring3タスクを追加する
void scheduler_add_task_user(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task       = (task_t*)kmalloc(sizeof(task_t));
    void*   kstack     = buddy_alloc(0);  // Ring0スタック（割り込み受け取り用）
    void*   ustack     = buddy_alloc(0);  // Ring3スタック

    task->page_table   = vmm_create_address_space();
    task->kstack_top   = (uint64_t)kstack + PAGE_SIZE;  // TSS RSP0に設定する値

    // Ring3 iretqフレーム: [rip, cs, rflags, rsp, ss]
    // CPL変化があるのでrsp/ssをフレームに含める（通常通り）
    uint64_t* sp = (uint64_t*)((uint64_t)ustack + PAGE_SIZE);
    uint64_t initial_rsp = (uint64_t)sp;
    *--sp = 0x23;            // ss  = User Data | RPL3
    *--sp = initial_rsp;     // rsp
    *--sp = 0x202;           // rflags: IF=1
    *--sp = 0x1B;            // cs  = User Code | RPL3
    *--sp = (uint64_t)entry; // rip

    task->rsp   = (uint64_t)sp;
    task->stack = ustack;
    task->state = 1;
    task->next  = 0;

    int i = 0;
    while (name[i] && i < 31) { task->name[i] = name[i]; i++; }
    task->name[i] = '\0';

    task_list[task_count++] = task;
}

void schedule() {
    if (task_count < 2) return;

    static int current_idx = 0;
    int next_idx = (current_idx + 1) % task_count;

    task_t* prev = task_list[current_idx];
    task_t* next = task_list[next_idx];
    current_idx  = next_idx;

    if (next->page_table) {
        vmm_switch(next->page_table);
    }

    // Ring3タスクに切り替える場合はTSSのRSP0を更新
    if (next->kstack_top) {
        tss_set_rsp0(next->kstack_top);
    }

    context_switch(prev, next);
}

int scheduler_get_task_count() {
    return task_count;
}

task_t* scheduler_get_task(int idx) {
    if (idx >= task_count) return 0;
    return task_list[idx];
}