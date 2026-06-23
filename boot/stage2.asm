[BITS 16]
[ORG 0x7E00]

start:


    ; start:の最初に追加
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov ax, 0x2401
    int 0x15
    ;カーソルを非表示にする
    mov ah, 0x01
    mov ch, 0x3F
    int 0x10
    ;load kernel from disk (セクタ34から34セクタ分を0x20000にロード)
    mov ah, 0x42
    mov dl, 0x80
    mov si, dap
    int 0x13
    jc disk_error

    ;get memory map
    ; メモリマップを0x500に書き込む
    mov di, 0x500
    xor ebx, ebx       ; 最初のエントリ
.e820_loop:
    mov eax, 0xE820
    mov ecx, 24        ; エントリサイズ
    mov edx, 0x534D4150 ; "SMAP"
    int 0x15
    jc .e820_done      ; エラーまたは終了
    add di, 24
    test ebx, ebx
    jnz .e820_loop
.e820_done:


    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

dap:
    db 0x10         ; DAPのサイズ
    db 0            ; 予約
    dw KERNEL_SECTORS ; 読むセクタ数
    dw 0x0000       ; 書き込み先オフセット
    dw 0x2000       ; 書き込み先セグメント (0x20000)
    dq 35           ; 開始LBAセクタ

disk_error:
    mov ah,0x0E
    mov al, 'E'
    int 0x10
    mov al, 'R'
    int 0x10
    mov al, 'R'
    int 0x10
    mov al, 'O'
    int 0x10
    mov al, 'R'
    int 0x10
    mov al, '!'
    int 0x10
    mov al ,'D'
    int 0x10
    mov al, 'I'
    int 0x10
    mov al, 'S'
    int 0x10
    mov al, 'K'
    int 0x10
    mov al, '!'
    hlt
gdt_start:
    dq 0
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
    ; 64-bit code segment (0x18)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 10101111b
    db 0x00
    ; 64-bit data segment (0x20)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 10101111b
    db 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start



[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov esp, 0x7C00         ; 安全のためESPを初期化

    mov edi, 0xB8000
    mov ecx, 80 * 25        ; 80列 * 25行
    mov ax, 0x0F20          ; 0x20=スペース, 0x0F=白文字黒背景
    rep stosw               ; ECX回繰り返してAXを書き込む

    mov esi, hello_msg
    call print_string

setup_long_mode:
    ; ページテーブルの初期化
    ; PML4 = 0x10000, PDPT = 0x11000, PD = 0x12000
    ; PTは使わず2MBラージページで1GBをマップ

    ; PML4, PDPT, PD領域(各4KB, 合計12KB)をゼロクリア
    mov edi, 0x10000
    mov ecx, 3072           ; 4 * 3072 = 12KB
    xor eax, eax
    rep stosd

    ; PML4[0] → PDPT
    mov eax, 0x11000
    or eax, 0x03
    mov [0x10000], eax

    ; PDPT[0] → PD
    mov eax, 0x12000
    or eax, 0x03
    mov [0x11000], eax

    ; PD: 512エントリ × 2MB = 1GB をラージページでマップ
    ; PD[i] = i * 0x200000 | 0x83 (Present + R/W + PageSize)
    mov ecx, 0
    .pd_loop:
        mov eax, ecx
        shl eax, 21          ; 2MBページ = 21ビットシフト
        or eax, 0x83         ; Present + Read/Write + PageSize(2MB)
        mov [0x12000 + ecx*8], eax
        inc ecx
        cmp ecx, 512
        jl .pd_loop

    ; CR3にPML4のアドレス(0x10000)を設定
    mov eax, 0x10000
    mov cr3, eax

    ;PAEを有効化
    mov eax, cr4
    or eax,(1 << 5) ; Set PAE bit
    mov cr4, eax

    ;EFERを有効化
    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or eax, (1 << 8) ; Set LME bit
    wrmsr

    ;CR0にPGビットを設定してページングを有効化
    mov eax, cr0
    or eax, (1 << 31) ; Set PG bit
    mov cr0, eax

    ;64ビットモードに移行
    jmp 0x18:long_mode


print_string:
    mov edi,0xB8000
.loop:
    lodsb
    test al, al
    jz .done
    mov [edi],al ;write character
    mov byte [edi+1],0x0F ;color
    add edi,2
    jmp .loop
.done:
    ret

hello_msg db 'Hello, Bootloader!', 0;
[BITS 64]
long_mode:
    mov byte [0xB8000], 'H'
    mov byte [0xB8001], 0xF0 ;白文字黒背景
    mov byte [0xB8002], 'i'
    mov byte [0xB8003], 0xF0
    mov byte [0xB8004], '!'
    mov byte [0xB8005], 0xF0
    mov byte [0xB8006], ' '
    mov byte [0xB8007], 0xF0
    mov byte [0xB8008], '6'
    mov byte [0xB8009], 0xF0
    mov byte [0xB800A], '4'
    mov byte [0xB800B], 0xF0
    mov byte [0xB800C], 'b'
    mov byte [0xB800D], 0xF0
    mov byte [0xB800E], 'i'
    mov byte [0xB800F], 0xF0
    mov byte [0xB8010], 't'
    mov byte [0xB8011], 0xF0

    mov rsp, 0x200000        ; RSP をカーネル用スタックに設定

    jmp 0x20000              ; カーネルエントリポイントへジャンプ
    