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
    char name[32];  // タスク名
    void*    stack;     // スタック領域
    int      state;     // RUNNING/READY/BLOCKED
    struct task* next;
} task_t;