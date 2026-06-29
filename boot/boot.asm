[BITS 16]
[ORG 0x7C00]

; FAT32 BPB用JMP (3バイト)
jmp short main
nop

; BPB領域 (オフセット3〜89): mkfs.fatが上書きする
times 87 db 0

; ブートコード (オフセット90)
main:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; BIOSが渡してきたドライブ番号(dl)を保存
    mov [boot_drive], dl

    ; STAGE2.BINをLBA=2から64セクタ(32KB)→ 0x7E00にロード
    mov ah, 0x42
    mov si, dap
    int 0x13
    jc  boot_error

    ; ドライブ番号をstage2に引き渡す (0x7000に置く)
    mov al, [boot_drive]
    mov [0x7000], al

    jmp 0x7E00

boot_error:
    mov si, msg_err
    call print_str
    jmp $

print_str:
    lodsb
    test al, al
    jz   .done
    mov  ah, 0x0E
    int  0x10
    jmp  print_str
.done:
    ret

msg_err    db 'Load error', 0x0D, 0x0A, 0
boot_drive db 0

; DAP構造体
dap:
    db  0x10        ; DAPサイズ
    db  0x00        ; 予約
    dw  64          ; セクタ数
    dw  0x7E00      ; バッファオフセット
    dw  0x0000      ; バッファセグメント
    dq  2           ; LBA=2
dap_drive equ 0     ; 未使用(dlレジスタで渡す)

times 510-($-$$) db 0
dw 0xAA55
