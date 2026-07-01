[BITS 16]
[ORG 0x7E00]

; =============================================================
;  Stage2: FAT32からKERNEL.BINをロードしてロングモードへ移行
;
;  メモリマップ:
;    0x0500        : e820メモリマップ
;    0x6000        : FAT読み込みバッファ (1セクタ)
;    0x6200        : RootDir読み込みバッファ (1セクタ)
;    0x7C00        : MBR / BPB (ここからBPBを読む)
;    0x7E00〜0x80F3: Stage2自身
;    0x10000〜12FFF: ページテーブル
;    0x20000〜     : KERNEL.BINロード先
; =============================================================

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; boot.asmが0x7000に書いたドライブ番号を保存
    mov al, [0x7000]
    mov [boot_drive], al

    ; A20ライン有効化
    mov ax, 0x2401
    int 0x15

    ; ---- BPBパラメータを変数に保存 ----
    mov si, 0x7C00

    movzx eax, word [si+14]     ; BPB_RsvdSecCnt
    mov [bpb_rsvd], ax

    mov al, [si+13]             ; BPB_SecPerClus
    mov [bpb_spc], al

    mov al, [si+16]             ; BPB_NumFATs
    mov [bpb_numfats], al

    mov eax, [si+36]            ; BPB_FATSz32
    mov [bpb_fatsz], eax

    mov eax, [si+44]            ; BPB_RootClus
    mov [bpb_rootclus], eax

    ; DataStartLBA = RsvdSecCnt + NumFATs * FATSz32
    movzx eax, word [bpb_rsvd]
    movzx ebx, byte [bpb_numfats]
    mov   ecx, [bpb_fatsz]
    imul  ecx, ebx
    add   eax, ecx
    mov   [data_start_lba], eax

    ; ---- RootDirの先頭セクタを0x6200にロード ----
    mov eax, [bpb_rootclus]
    call cluster_to_lba

    mov [dap_lba], eax
    mov word [dap_count], 1
    mov word [dap_seg], 0x0000
    mov word [dap_off], 0x6200
    call disk_read

    ; ---- ディレクトリエントリからKERNEL.BINを探す ----
    mov si, 0x6200
    mov cx, 16
.search_kernel:
    cmp byte [si], 0x00
    je  .kernel_not_found
    cmp byte [si], 0xE5
    je  .next_entry
    ; ボリュームラベル(attr=0x08)はスキップ
    test byte [si+11], 0x08
    jnz .next_entry
    push si
    push cx
    mov di, kernel_name
    mov cx, 11
    repe cmpsb
    pop cx
    pop si
    je  .kernel_found
.next_entry:
    add si, 32
    loop .search_kernel
.kernel_not_found:
    mov si, msg_notfound
    call print_str
    jmp $

.kernel_found:
    movzx eax, word [si+20]     ; クラスタ上位2バイト
    shl   eax, 16
    movzx ebx, word [si+26]     ; クラスタ下位2バイト
    or    eax, ebx
    mov   [kernel_clus], eax

    ; ---- KERNEL.BINをクラスタチェーンで0x20000にロード ----
    ; セグメント0x2000, オフセット0x0000 → 物理0x20000
    mov word [load_seg], 0x2000
    mov word [load_off], 0x0000

.load_chain:
    mov eax, [kernel_clus]
    cmp eax, 0x0FFFFFF8
    jae .load_done

    call cluster_to_lba

    movzx ecx, byte [bpb_spc]
.load_sector:
    push ecx
    push eax

    mov [dap_lba], eax
    mov word [dap_count], 1
    mov ax, [load_seg]
    mov [dap_seg], ax
    mov ax, [load_off]
    mov [dap_off], ax
    call disk_read

    ; バッファポインタを1セクタ(512B)進める
    mov ax, [load_off]
    add ax, 512
    jnc .no_overflow
    ; セグメントを0x20進める(0x20*16=512)
    mov ax, [load_seg]
    add ax, 0x20
    mov [load_seg], ax
    mov word [load_off], 0
    jmp .sector_done
.no_overflow:
    mov [load_off], ax
.sector_done:
    pop eax
    inc eax
    pop ecx
    loop .load_sector

    ; 次のクラスタをFATから取得
    mov eax, [kernel_clus]
    call read_fat_entry
    mov [kernel_clus], eax
    jmp .load_chain

.load_done:
    ; ---- メモリマップ取得 (e820) ----
    ; エントリを0x500に書く。最大128エントリ(3072バイト)で上限打ち切り
    mov di, 0x500
    xor ebx, ebx
    mov bp, 0               ; エントリ数カウンタ
.e820_loop:
    mov eax, 0xE820
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc  .e820_done          ; エラーまたは終了
    cmp eax, 0x534D4150     ; BIOSが"SMAP"を返しているか確認
    jne .e820_done
    add di, 24
    inc bp
    cmp bp, 128             ; 上限チェック
    jae .e820_done
    test ebx, ebx
    jnz .e820_loop
.e820_done:
    ; エントリ数を0x4F8に保存（カーネルが参照できるよう）
    mov [0x4F8], bp

    ; ---- GDT → Protected Mode → Long Mode ----
    lgdt [gdt_descriptor]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

; =============================================================
;  cluster_to_lba: eax=クラスタ番号 → eax=LBA
; =============================================================
cluster_to_lba:
    sub   eax, 2
    movzx ecx, byte [bpb_spc]
    imul  eax, ecx
    add   eax, [data_start_lba]
    ret

; =============================================================
;  read_fat_entry: eax=クラスタ番号 → eax=次クラスタ番号
;  FATセクタを0x6000にロード (stage2と重ならない安全な領域)
; =============================================================
read_fat_entry:
    push ebx
    push ecx
    mov  ecx, eax
    shl  ecx, 2                 ; byte offset in FAT = cluster * 4
    mov  eax, ecx
    shr  eax, 9                 ; FAT内セクタ番号
    movzx ebx, word [bpb_rsvd]
    add  eax, ebx               ; 実LBA
    mov [dap_lba], eax
    mov word [dap_count], 1
    mov word [dap_seg], 0x0000
    mov word [dap_off], 0x6000
    call disk_read
    and  ecx, 0x1FF             ; セクタ内バイトオフセット
    mov  eax, [0x6000 + ecx]
    and  eax, 0x0FFFFFFF
    pop  ecx
    pop  ebx
    ret

; =============================================================
;  disk_read: DAPを使ってint 0x13拡張読み込み
; =============================================================
disk_read:
    pusha
    mov ah, 0x42
    mov dl, [boot_drive]    ; BIOSから受け取った実際のドライブ番号を使う
    mov si, dap
    int 0x13
    jc  disk_error
    popa
    ret

disk_error:
    mov si, msg_diskerr
    call print_str
    jmp $

; =============================================================
;  print_str: BIOSでSI=文字列を表示
; =============================================================
print_str:
    lodsb
    test al, al
    jz   .done
    mov  ah, 0x0E
    int  0x10
    jmp  print_str
.done:
    ret

; ---- データ ----
boot_drive     db 0
kernel_name    db 'KERNEL  BIN'
msg_notfound   db 'KERNEL.BIN not found', 0x0D, 0x0A, 0
msg_diskerr    db 'Disk error', 0x0D, 0x0A, 0

bpb_rsvd      dw 0
bpb_spc       db 0
bpb_numfats   db 0
bpb_fatsz     dd 0
bpb_rootclus  dd 0
data_start_lba dd 0
kernel_clus    dd 0
load_seg       dw 0
load_off       dw 0

; DAP (Disk Address Packet) - stage2の最後の方に配置
dap:
    db 0x10
    db 0
dap_count:  dw 1
dap_off:    dw 0
dap_seg:    dw 0
dap_lba:    dq 0

; =============================================================
[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7C00

setup_long_mode:
    ; ページテーブル (PML4=0x10000, PDPT=0x11000, PD=0x12000)
    mov edi, 0x1000
    mov ecx, 3072
    xor eax, eax
    rep stosd

    ; PML4[0] → PDPT (Identity Mapping)
    mov eax, 0x11000
    or  eax, 0x03
    mov [0x10000], eax

    ; PML4[256] → PDPT (Higher Half)
    mov eax, 0x11000
    or  eax, 0x03
    mov [0x10000 + 256*8], eax

    ; PDPT[0] → PD
    mov eax, 0x12000
    or  eax, 0x03
    mov [0x11000], eax

    ; PD: 512 × 2MB = 1GB
    mov ecx, 0
.pd_loop:
    mov eax, ecx
    shl eax, 21
    or  eax, 0x83
    mov [0x12000 + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jl  .pd_loop

    mov eax, 0x10000
    mov cr3, eax

    mov eax, cr4
    or  eax, (1 << 5)
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)
    wrmsr

    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax

    jmp 0x18:long_mode

; ---- GDT ----
gdt_start:
    dq 0
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00  ; 32-bit code (0x08)
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00  ; 32-bit data (0x10)
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 10101111b, 0x00  ; 64-bit code (0x18)
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 10101111b, 0x00  ; 64-bit data (0x20)
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 64]
long_mode:
    mov rsp, 0x200000
    jmp 0x20000
