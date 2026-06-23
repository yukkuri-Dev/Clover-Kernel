[BITS 64]
global _start
extern kernel_main
_start:
    mov rsp, 0x200000
    call kernel_main
.halt:
    cli
    hlt
    jmp .halt
