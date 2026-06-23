[BITS 16]
[ORG 0x7C00]

start:
    ;init
    xor ax, ax          ;int ax = 0
    mov ds, ax          ;ds = ax
    mov es, ax          ;es = ax
    mov ss, ax          ;ss = ax
    mov sp, 0x7C00      ;int sp = 0x7C00


    ;load stage 2
    mov ah, 0x02        ;AH = 0x02 (Read Sectors)
    mov al, 34          ;AL = 34 (Read 34 sectors)
    mov ch,0
    mov cl,2            ;CL = 2 (Read from sector 2)
    mov dh,0
    mov dl,0x80         ;DL = 0x80 (First hard disk)
    mov bx,0x7E00       ;BX = 0x7E00 (Load address)
    int 0x13            ;Call BIOS interrupt
    jc load_error       ;If carry flag is set, there was an error
    jmp 0x7E00          ;Jump to the loaded stage 2

load_error:
    hlt                 ;stop


times 510-($-$$) db 0 ;Fill the rest of the sector with zeros
dw 0xAA55            ;Boot signature