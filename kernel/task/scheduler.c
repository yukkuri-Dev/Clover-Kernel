#include "scheduler.h"
#include "../memmgr/slab.h"
#include "../memmgr/buddy.h"
#include "../memmgr/vmm.h"
#include "../gdt/gdt.h"

extern void context_switch(task_t* current, task_t* next);
extern void task_wrapper();
extern void task_launcher();

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
    kernel_task->ustack     = 0;
    set_name(kernel_task, "kernel");
    current_task = kernel_task;
    task_list_append(kernel_task);
}

void scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task  = (task_t*)kmalloc(sizeof(task_t));
    void*   stack = buddy_alloc(0);
    if (!task || !stack) return;

    task->page_table = 0;

    uint64_t* sp = (uint64_t*)((uint64_t)stack + PAGE_SIZE);
    uint64_t stack_top = (uint64_t)sp;
    // iretqフレーム（task_launcherがiretqする）
    *--sp = 0x10;                      // ss
    *--sp = stack_top;                 // rsp
    *--sp = 0x202;                     // rflags (IF=1)
    *--sp = 0x08;                      // cs
    *--sp = (uint64_t)task_wrapper;    // rip
    // context_switchのretで飛ぶアドレス
    *--sp = (uint64_t)task_launcher;

    task->rdi        = (uint64_t)entry;
    task->rsp        = (uint64_t)sp;
    task->kstack_top = 0;
    task->ring       = 0;
    task->stack      = stack;
    task->ustack     = 0;
    task->state      = 1;
    set_name(task, name);
    task_list_append(task);
}

// ユーザースタックの仮想アドレス先頭（4KBの1ページ）
#define USER_STACK_VIRT_BASE 0x0000700000000000ULL
// ユーザーコードの仮想アドレス先頭。
// ブートローダが2MBラージページでマップしている低位1GB(PML4[0])と
// 衝突しないよう、専用のPML4エントリ(PML4[192])を使う。
// （vmm_map_pageは4KBページ前提でラージページPDを分割できないため）
#define USER_CODE_VIRT_BASE  0x0000600000000000ULL

extern void vga_print(const char*);
extern void vga_print_hex(uint64_t);

// linker.ld が定義する .user_text セクションの範囲
extern char __user_text_start[];
extern char __user_text_end[];

void scheduler_add_task_user(const char* name, void (*entry)(), uint64_t stack_size __attribute__((unused))) {
    task_t* task   = (task_t*)kmalloc(sizeof(task_t));
    void*   kstack = buddy_alloc(0);  // Ring0割り込み用カーネルスタック
    void*   ustack = buddy_alloc(0);  // Ring3ユーザースタック（物理ページ）
    void*   ucode  = buddy_alloc(0);  // Ring3ユーザーコード（物理ページ）
    if (!task || !kstack || !ustack || !ucode) return;

    // .user_text セクション全体をユーザーコードページへコピーする。
    // entry はカーネルhigh halfのアドレスなので、セクション先頭からの
    // オフセットを求め、ユーザー仮想アドレスへ変換する。
    uint64_t sec_size  = (uint64_t)__user_text_end - (uint64_t)__user_text_start;
    char* src = __user_text_start;
    char* dst = (char*)ucode;
    for (uint64_t i = 0; i < sec_size && i < PAGE_SIZE; i++)
        dst[i] = src[i];

    uint64_t entry_off  = (uint64_t)entry - (uint64_t)__user_text_start;
    uint64_t entry_virt = USER_CODE_VIRT_BASE + entry_off;

    // 専用アドレス空間を作り、ユーザースタック/コードをUSERページとしてマップ
    page_table_t pt = vmm_create_address_space();
    vmm_map_page(pt, USER_STACK_VIRT_BASE, (uint64_t)ustack,
                 PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    vmm_map_page(pt, USER_CODE_VIRT_BASE, (uint64_t)ucode,
                 PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    task->page_table = pt;
    task->kstack_top = (uint64_t)kstack + PAGE_SIZE;  // 割り込み時のTSS RSP0

    // 初回起動用のiretqフレーム（Ring0 kstack上に積む）
    // task_launcher の iretq が Ring3 へ特権レベルを落として飛ぶ
    uint64_t* sp = (uint64_t*)((uint64_t)kstack + PAGE_SIZE);
    uint64_t ustack_top = USER_STACK_VIRT_BASE + PAGE_SIZE;
    *--sp = 0x1B;                      // ss  = UData(0x18) | RPL3
    *--sp = ustack_top;                // rsp = ユーザースタック仮想アドレス
    *--sp = 0x202;                     // rflags (IF=1)
    *--sp = 0x23;                      // cs  = UCode(0x20) | RPL3
    *--sp = entry_virt;                // rip = ユーザー仮想アドレス上のエントリ
    *--sp = (uint64_t)task_launcher;   // context_switchのretで飛ぶ先

    task->rdi    = 0;
    task->rsp    = (uint64_t)sp;
    task->ring   = 3;
    task->stack  = kstack;
    task->ustack = ustack;
    task->state  = 1;
    set_name(task, name);
    task_list_append(task);
}

void task_exit() {
    __asm__ volatile ("cli");

    task_t* dead = current_task;

    // リストから自分を外す
    task_t* prev = 0;
    task_t* t    = task_head;
    while (t) {
        if (t == dead) {
            if (prev)
                prev->next = t->next;
            else
                task_head = t->next;
            if (task_tail == t)
                task_tail = prev;
            task_count--;
            break;
        }
        prev = t;
        t    = t->next;
    }

    // 次に走らせる生存タスクを決める（自分は外れているので head から）
    task_t* next = dead->next;
    if (!next) next = task_head;
    if (!next) {
        // 走らせるタスクがなくなった：そのまま停止
        while (1) __asm__ volatile ("hlt");
    }

    current_task = next;

    // 先に next のアドレス空間へ切り替える。
    // これで dead->page_table が現在のCR3でなくなり、安全に解放できる。
    if (next->page_table)
        vmm_switch(next->page_table);
    else
        vmm_switch((page_table_t)0x10000);

    if (next->kstack_top)
        tss_set_rsp0(next->kstack_top);

    // 注意: dead->stack は今まさに実行中のスタックなので、ここでは解放しない
    //       (context_switch 直前に解放すると割り込み等で再利用され破壊される)。
    //       ustack / page_table は実行中スタックではないので解放してよい。
    if (dead->ustack)
        buddy_free(dead->ustack, 0);
    if (dead->page_table)
        buddy_free((void*)dead->page_table, 0);
    // TODO: dead->stack / dead->kstack / task構造体は遅延解放(reaper)で回収する

    // 明示的に next へ切り替える。dead は捨てるので保存先はダミー。
    static task_t dummy;
    context_switch(&dummy, next);
    // ここには戻ってこない
    __builtin_unreachable();
}

// 次のタスクを選んでcurrent_taskを更新、prev/nextを返す
// スイッチ自体はpit_handlerのASM側で行う
task_t* schedule_next(task_t** out_prev) {
    if (task_count < 2) return 0;

    task_t* next = current_task->next;
    if (!next) next = task_head;

    *out_prev    = current_task;
    current_task = next;

    // アドレス空間切り替え:
    //   Ring3タスク → 専用PML4
    //   Ring0タスク → ブートローダのカーネルPML4(0x10000)
    if (next->page_table)
        vmm_switch(next->page_table);
    else
        vmm_switch((page_table_t)0x10000);

    if (next->kstack_top)
        tss_set_rsp0(next->kstack_top);

    return next;
}

task_t* get_current_task() {
    return current_task;
}

// syscall_entry が使う：現在タスクのカーネルスタック先頭。
// Ring3タスクは kstack_top、Ring0タスク(あれば)はstack上端を返す。
uint64_t syscall_kstack_top() {
    if (current_task->kstack_top)
        return current_task->kstack_top;
    // Ring0タスクからのsyscallは想定外だが、安全に現在のstack上端を返す
    if (current_task->stack)
        return (uint64_t)current_task->stack + PAGE_SIZE;
    return 0;
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
