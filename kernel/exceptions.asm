[BITS 64]
global gp_fault_handler
global pf_handler

gp_fault_handler:
    pop rax              ; エラーコードを捨てる（先に捨てる）
    mov rdi, [rsp + 0]   ; rip
    mov rsi, [rsp + 8]   ; cs
    mov rdx, rax         ; エラーコードも渡す
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
    push rax
    mov al, 0x20
    out 0x20, al
    pop rax

    ; out_prev 用スロットをスタックに確保
    sub  rsp, 8
    mov  rdi, rsp
    call schedule_next   ; rax = next, [rsp] = prev
    mov  rdi, [rsp]      ; rdi = prev
    add  rsp, 8

    test rax, rax
    jz   .no_switch

    mov  rsi, rax        ; rsi = next
    call context_switch
    iretq

.no_switch:
    iretq

extern gp_fault_handler_c
extern pf_handler_c

