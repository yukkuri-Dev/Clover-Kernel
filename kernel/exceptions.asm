[BITS 64]
global gp_fault_handler
global pf_handler

gp_fault_handler:
    mov rdi, [rsp + 8]   ; rip
    mov rsi, [rsp + 16]  ; cs
    pop rax              ; エラーコードを捨てる
    call gp_fault_handler_c
    iretq

pf_handler:
    pop rax              ; エラーコードを捨てる
    mov rdi, cr2         ; cr2 = フォルトアドレス
    mov rsi, [rsp + 8]   ; cs
    call pf_handler_c
    iretq

global df_handler
extern df_handler_c
df_handler:
    ; エラーコードを捨てない
    mov rdi, [rsp + 8]  ; rip
    pop rax
    call df_handler_c
    iretq

global keyboard_handler
extern keyboard_handler_c
keyboard_handler:
    call keyboard_handler_c
    mov al, 0x20
    out 0x20, al
    iretq

global pit_handler
extern schedule_next
extern context_switch

pit_handler:
    ; EOI を最初に送る
    push rax
    mov al, 0x20
    out 0x20, al
    pop rax

    ; schedule_next(&prev) を呼ぶ
    ; caller-savedをここでは保存しない → context_switch側で管理
    push rbx
    sub  rsp, 8
    mov  rdi, rsp        ; &out_prev
    call schedule_next   ; rax = next (0なら切り替えなし)
    mov  rbx, [rsp]      ; rbx = prev
    add  rsp, 8

    test rax, rax
    jz   .no_switch

    ; context_switch(prev, next) を tail-call
    ; ただしcallするとリターンアドレスが積まれるので
    ; rbxとリターン先をセットしてjmp
    mov  rdi, rbx        ; prev
    mov  rsi, rax        ; next
    pop  rbx             ; rbxを復元
    jmp  context_switch  ; context_switch内でiretq

.no_switch:
    pop rbx
    iretq

extern gp_fault_handler_c
extern pf_handler_c

