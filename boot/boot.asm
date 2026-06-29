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

    ; STAGE2.BINはFAT32予約領域のLBA=2から書き込まれている
    ; (LBA=0:MBR LBA=1:FSInfo LBA=6:BkBoot, LBA=2〜5,7〜31が空き)
    ; 固定LBA=2から29セクタを0x7E00にロード

    mov ah, 0x42
    mov dl, 0x80
    mov si, dap
    int 0x13
    jc  boot_error

    jmp 0x7E00

boot_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, 'R'
    int 0x10
    hlt

; DAP構造体
dap:
    db  0x10        ; DAPサイズ
    db  0x00        ; 予約
    dw  64          ; セクタ数 (64 * 512 = 32KB)
    dw  0x7E00      ; バッファオフセット
    dw  0x0000      ; バッファセグメント
    dq  2           ; LBA=2 (固定、FAT32予約領域の空きセクタ)

times 510-($-$$) db 0
dw 0xAA55
