#include "scheduler.h"
#include "../memmgr/slab.h"
#include "../memmgr/buddy.h"
#include "../memmgr/vmm.h"
#include "../gdt/gdt.h"

extern void context_switch(task_t* current, task_t* next);
extern void task_wrapper();

static task_t* task_head    = 0;  // リスト先頭
static task_t* task_tail    = 0;  // リスト末尾（追加用）
static task_t* current_task = 0;
static int     task_count   = 0;

static void task_list_append(task_t* task) {
    task->next = 0;
    if (!task_head) {
        task_head = task_tail = task;
    } else {
        task_tail->next = task;
        task_tail = task;
    }
    task_count++;
}

static void set_name(task_t* task, const char* name) {
    int i = 0;
    while (name[i] && i < 31) { task->name[i] = name[i]; i++; }
    task->name[i] = '\0';
}

void scheduler_init() {
    task_t* kernel_task   = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->state      = 1;
    kernel_task->page_table = 0;
    kernel_task->kstack_top = 0;
    kernel_task->ring       = 0;
    kernel_task->stack      = 0;
    set_name(kernel_task, "kernel");
    current_task = kernel_task;
    task_list_append(kernel_task);
}

void scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task  = (task_t*)kmalloc(sizeof(task_t));
    void*   stack = buddy_alloc(0);

    task->page_table = 0;

    uint64_t* sp = (uint64_t*)((uint64_t)stack + PAGE_SIZE);
    uint64_t stack_top = (uint64_t)sp;
    *--sp = 0x10;              // ss
    *--sp = stack_top;         // rsp (iretq後に使うrsp = スタック先頭)
    *--sp = 0x202;             // rflags (IF=1)
    *--sp = 0x08;              // cs
    *--sp = (uint64_t)task_wrapper; // rip

    task->rdi        = (uint64_t)entry;
    task->rsp        = (uint64_t)sp;
    task->kstack_top = 0;
    task->ring       = 0;
    task->stack      = stack;
    task->state      = 1;
    set_name(task, name);
    task_list_append(task);
}

void scheduler_add_task_user(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task   = (task_t*)kmalloc(sizeof(task_t));
    void*   kstack = buddy_alloc(0);
    void*   ustack = buddy_alloc(0);

    task->page_table = vmm_create_address_space();
    task->kstack_top = (uint64_t)kstack + PAGE_SIZE;

    uint64_t* sp = (uint64_t*)((uint64_t)ustack + PAGE_SIZE);
    uint64_t ustack_top = (uint64_t)sp;
    *--sp = 0x23;              // ss
    *--sp = ustack_top;        // rsp (iretq後に使うrsp = スタック先頭)
    *--sp = 0x202;             // rflags (IF=1)
    *--sp = 0x1B;              // cs
    *--sp = (uint64_t)task_wrapper; // rip

    task->rdi   = (uint64_t)entry;
    task->rsp   = (uint64_t)sp;
    task->ring  = 3;
    task->stack = ustack;
    task->state = 1;
    set_name(task, name);
    task_list_append(task);
}

void task_exit() {
    __asm__ volatile ("cli");

    task_t* prev = 0;
    task_t* t    = task_head;
    while (t) {
        if (t == current_task) {
            if (prev)
                prev->next = t->next;
            else
                task_head = t->next;
            if (task_tail == t)
                task_tail = prev;
            task_count--;
            if (t->stack)
                buddy_free(t->stack, 0);
            if (t->kstack_top)
                buddy_free((void*)(t->kstack_top - PAGE_SIZE), 0);
            if (t->page_table)
                buddy_free((void*)t->page_table, 0);
            kfree(t);
            break;
        }
        prev = t;
        t    = t->next;
    }

    __asm__ volatile ("sti");
    while (1) __asm__ volatile ("hlt");
}

// 次のタスクを選んでcurrent_taskを更新、prev/nextを返す
// スイッチ自体はpit_handlerのASM側で行う
task_t* schedule_next(task_t** out_prev) {
    if (task_count < 2) return 0;

    task_t* next = current_task->next;
    if (!next) next = task_head;

    *out_prev    = current_task;
    current_task = next;

    if (next->page_table)
        vmm_switch(next->page_table);

    if (next->kstack_top)
        tss_set_rsp0(next->kstack_top);

    return next;
}

task_t* get_current_task() {
    return current_task;
}

int scheduler_get_task_count() {
    return task_count;
}

task_t* scheduler_get_task(int idx) {
    task_t* t = task_head;
    for (int i = 0; i < idx && t; i++)
        t = t->next;
    return t;
}
