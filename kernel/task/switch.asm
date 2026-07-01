[BITS 64]
global context_switch
global task_wrapper
global task_launcher
extern task_exit

task_wrapper:
    call rdi
    call task_exit
    ud2

task_launcher:
    iretq

; task_t offsets:
;   0:   rsp
;  32:   rbx
;  64:   rdi
; 104:   r12
; 112:   r13
; 120:   r14
; 128:   r15
; 136:   rbp
; 144:   page_table

context_switch:
    mov [rdi + 32],  rbx
    mov [rdi + 104], r12
    mov [rdi + 112], r13
    mov [rdi + 120], r14
    mov [rdi + 128], r15
    mov [rdi + 136], rbp
    mov [rdi + 0],   rsp

    mov rbx, [rsi + 32]
    mov r12, [rsi + 104]
    mov r13, [rsi + 112]
    mov r14, [rsi + 120]
    mov r15, [rsi + 128]
    mov rbp, [rsi + 136]
    mov rdi, [rsi + 64]
    mov rsp, [rsi + 0]

    ret
