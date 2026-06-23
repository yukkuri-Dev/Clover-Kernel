[BITS 64]
global gp_fault_handler
global pf_handler

gp_fault_handler:
    pop rax             ; エラーコードを捨てる
    call gp_fault_handler_c
    iretq

pf_handler:
    pop rax             ; エラーコードを捨てる
    call pf_handler_c
    iret

global keyboard_handler
keyboard_handler:
    ; キースキャンコードを読む
    in al, 0x60


    ; VGAの特定の場所に表示
    mov byte [0xB8080], al   ; スキャンコードの値
    mov byte [0xB8081], 0x0A ; 緑
    




    ; EOIを送る
    mov al, 0x20
    out 0x20, al
    iretq

global pit_handler
extern pit_timer_handler

pit_handler:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    call pit_timer_handler
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    iretq

extern gp_fault_handler_c
extern pf_handler_c

