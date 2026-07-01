[BITS 64]
global syscall_entry
extern syscall_dispatch
extern syscall_kstack_top   ; 現在のタスクのカーネルスタック先頭を返すC関数

; SYSCALL命令でRing3から入ってくる。
;   rax = syscall番号
;   rdi=arg1, rsi=arg2, rdx=arg3, r10=arg4, r8=arg5, r9=arg6  (Linux syscall ABI)
;   rcx = ユーザーのrip（SYSCALLが保存、SYSRETで復帰に使う・破壊厳禁）
;   r11 = ユーザーのrflags（同上）
;
; syscall_dispatch(num, a1, a2, a3, a4, a5) は System V ABI:
;   rdi=num, rsi=a1, rdx=a2, rcx=a3, r8=a4, r9=a5
;
; SYSCALLはスタックを切り替えないため、ここで明示的にカーネルスタックへ
; 切り替える。これにより sys_exit→task_exit が vmm_switch でユーザー空間を
; 外してもスタックが生きている（ユーザースタックは専用アドレス空間にしか
; マップされていないため、CR3切替後に触ると#PF→#DFになる）。
; SFMASKでIF=0のためsyscall中は割り込みが入らず、kstackの自己上書きは起きない。

section .data
align 8
saved_user_rsp: dq 0

section .text
syscall_entry:
    ; ユーザーrspを退避（callee/呼び出しでrcx等が壊れる前に最小限だけ）
    mov  [rel saved_user_rsp], rsp
    mov  [rel saved_user_rcx], rcx
    mov  [rel saved_user_r11], r11
    mov  [rel saved_user_rax], rax

    ; カーネルスタックへ切り替える（現在タスクの kstack_top を取得）
    ; 引数レジスタを壊さないよう、ユーザー引数は退避後に詰め替える。
    push rdi
    push rsi
    push rdx
    push r8
    push r9
    push r10
    call syscall_kstack_top      ; rax = kstack_top
    mov  r11, rax                ; r11 = kstack_top（一時）
    pop  r10
    pop  r9
    pop  r8
    pop  rdx
    pop  rsi
    pop  rdi

    ; ここまでのpush/popはまだユーザースタック上。kstackへ移る。
    mov  rsp, r11                ; rsp = カーネルスタック先頭

    mov  rax, [rel saved_user_rax]

    ; 引数を Linux ABI → System V ABI へ詰め替える
    ; num=rax, a1=rdi, a2=rsi, a3=rdx, a4=r10, a5=r8
    mov  r9,  r8                 ; a5
    mov  r8,  r10                ; a4
    mov  rcx, rdx                ; a3
    mov  rdx, rsi                ; a2
    mov  rsi, rdi                ; a1
    mov  rdi, rax                ; num
    call syscall_dispatch        ; 戻り値 rax

    ; ユーザースタックとSYSRET用レジスタを復元
    mov  rsp, [rel saved_user_rsp]
    mov  rcx, [rel saved_user_rcx]
    mov  r11, [rel saved_user_r11]
    o64 sysret                   ; REX.W付きsysret（NASMは'sysretq'非対応）

section .data
align 8
saved_user_rcx: dq 0
saved_user_r11: dq 0
saved_user_rax: dq 0
