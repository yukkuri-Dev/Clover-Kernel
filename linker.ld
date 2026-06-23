ENTRY(_start)
SECTIONS {
    . = 0x20000;
    .text : {
        build/kernel.asm.o(.text)  /* _start を必ず先頭に */
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
