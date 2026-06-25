[BITS 64]
global context_switch

; context_switch(task_t* current, task_t* next)
; rdi = current,  rsi = next
;
; 全タスクのスタックは「iretqで再開できる形」で保存される。
; 新規タスクは scheduler_add_task でそのフレームを初期化済み。
;
; task_t offsets:
;   0:   rsp  (iretqフレーム [rip/cs/rflags/rsp/ss] のトップを指す)
;   32:  rbx,  104: r12,  112: r13,  120: r14,  128: r15,  136: rbp

context_switch:
    ; --- current のスタックに iretq フレームを積む ---
    ; iretq フレーム: [rip, cs, rflags, rsp, ss]  (rip が最上位)
    ; ss
    push 0x10              ; ss = Kernel Data
    ; rsp: この context_switch から schedule に戻った後の rsp
    ;      = context_switch 呼び出し前の rsp (リターンアドレスを除いた値)
    mov rax, rsp
    add rax, 8*2          ; push した ss(8) + リターンアドレス(8)
    push rax
    ; rflags: IF=1 にして割り込み許可を保証
    pushfq
    pop rax
    or  rax, 0x200
    push rax
    ; cs
    push 0x08              ; cs = Kernel Code (新GDT)
    ; rip: schedule 内のリターンアドレス = スタック上の [rsp + 4*8] の位置
    ;      iretqフレーム4つ(ss/rsp/rflags/cs) を積んだ後、リターンアドレスはその下にある
    mov rax, [rsp + 4*8]
    push rax

    ; callee-saved を保存
    mov [rdi + 32],  rbx
    mov [rdi + 104], r12
    mov [rdi + 112], r13
    mov [rdi + 120], r14
    mov [rdi + 128], r15
    mov [rdi + 136], rbp
    mov [rdi + 0],   rsp   ; iretqフレームのトップ

    ; --- next を復元して iretq で起動 ---
    mov rbx, [rsi + 32]
    mov r12, [rsi + 104]
    mov r13, [rsi + 112]
    mov r14, [rsi + 120]
    mov r15, [rsi + 128]
    mov rbp, [rsi + 136]
    mov rsp, [rsi + 0]
    iretq
