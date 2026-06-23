ENTRY(_start)
SECTIONS {
    . = 0xFFFF800000020000;
    .text : {
        build/kernel.asm.o(.text)  /* _start を必ず先頭に */
        *(.text)
    }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
