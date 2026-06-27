[BITS 64]
global context_switch
global task_wrapper
extern task_exit

; task_wrapper: iretq後にrdiにエントリ関数ポインタが入っている状態で呼ばれる
task_wrapper:
    call rdi       ; エントリ関数を呼ぶ
    call task_exit ; returnしたらtask_exitへ
    ud2            ; ここには来ないはず

; context_switch(task_t* current, task_t* next)
; rdi = current,  rsi = next
;
; task_t offsets:
;   0:   rsp, 32: rbx, 104: r12, 112: r13, 120: r14, 128: r15, 136: rbp
;   160: ring (uint8_t)  0=Ring0, 3=Ring3

context_switch:
    ; --- current の ring を見てiretqフレームを積む ---
    movzx rax, byte [rdi + 160]
    test  rax, rax
    jnz   .save_ring3

.save_ring0:
    mov rax, [rsp]         ; リターンアドレス(rip)を先に退避
    ; iretqフレーム5要素を積む: rip, cs, rflags, rsp, ss
    ; 積む前のrspは [rsp+8] (リターンアドレスの上 = 呼び出し元のrsp)
    mov rcx, rsp
    add rcx, 8              ; 呼び出し元のrsp（リターンアドレス分だけ上）
    push 0x10              ; ss
    push rcx               ; rsp
    pushfq
    pop rcx
    or  rcx, 0x200
    push rcx               ; rflags | IF
    push 0x08              ; cs
    push rax               ; rip
    jmp .save_regs

.save_ring3:
    mov rax, [rsp]         ; リターンアドレス(rip)を先に退避
    mov rcx, rsp
    add rcx, 8
    push 0x23              ; ss
    push rcx               ; rsp
    pushfq
    pop rcx
    or  rcx, 0x200
    push rcx               ; rflags | IF
    push 0x1B              ; cs
    push rax               ; rip

.save_regs:
    mov [rdi + 32],  rbx
    mov [rdi + 104], r12
    mov [rdi + 112], r13
    mov [rdi + 120], r14
    mov [rdi + 128], r15
    mov [rdi + 136], rbp
    mov [rdi + 0],   rsp

    ; --- next を復元して iretq で起動 ---
    mov rbx, [rsi + 32]
    mov r12, [rsi + 104]
    mov r13, [rsi + 112]
    mov r14, [rsi + 120]
    mov r15, [rsi + 128]
    mov rbp, [rsi + 136]
    mov rdi, [rsi + 64]   ; rdi = task->rdi (task_wrapper へのエントリ関数)
    mov rsp, [rsi + 0]
    iretq
