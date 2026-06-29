#include "../kernelstd/types.h"
#include "../memmgr/vmm.h"
typedef struct task {
    uint64_t rsp;       // スタックポインタ
    uint64_t rip;       // 次に実行する命令
    uint64_t rflags;
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rbp;
    
    page_table_t page_table; // ページテーブルの物理アドレス
    uint64_t kstack_top;     // Ring0スタック先頭（Ring3タスクのTSS RSP0用、Ring0タスクは0）
    uint8_t ring;            // 0 = Ring0, 3 = Ring3
    char name[32];
    void*    stack;          // Ring0タスク: カーネルスタック / Ring3タスク: kstack(Ring0割り込みスタック)
    void*    ustack;         // Ring3タスクのユーザースタック物理ページ（Ring0タスクは0）
    int      state;
    struct task* next;
} task_t;