ENTRY(_start)
SECTIONS {
    . = 0xFFFF800000020000;
    .text : {
        build/kernel/kernel.asm.o(.text)  /* _start を必ず先頭に */
        *(.text)
    }
    /* Ring3で実行されるユーザーコード。実行時にlow halfへコピーするため
       開始/終了シンボルでサイズを取得できるようにする。 */
    . = ALIGN(16);
    __user_text_start = .;
    .user_text : { *(.user_text) }
    __user_text_end = .;
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
