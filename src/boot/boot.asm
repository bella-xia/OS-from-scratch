ORG 0x7c00
BITS 16 ;in the 16-bit real-mode

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

_start:
    jmp short start
    nop

times 33 db 0

start: 
    jmp 0:step2 ;change the code segmentd

step2:
    cli ;clear interrupt flags
    mov ax, 0x00
    mov ds, ax ;move 0x7c0 to data segment
    mov es, ax ;move 0x7c0 to extra segment
    mov ss, ax ;move 0x00 to stack segment
    mov sp, 0x7c00 ;move the stack pointer to the address (so basically no segment)
    sti ;enable interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32

; GDT
gdt_start:
gdt_null:
    dd 0x00
    dd 0x00

; offset 0x8
gdt_code: ;CS SHOULD POINT TO THIS
    dw 0xffff ;segment limit first 0-15 bits
    dw 0 ;base 0-15 bits
    db 0 ;base 16-23 bits
    db 0x9a ;access byte
    db 11001111b ;high 4 bit flags and low 4 bit flags
    db 0 ;base 24-31 bits

; offset 0x10
gdt_data:   ; DS, SS, ES, FS, GS
    dw 0xffff ;segment limit first 0-15 bits
    dw 0 ;base 0-15 bits
    db 0 ;base 16-23 bits
    db 0x92 ;access byte
    db 11001111b ;high 4 bit flags and low 4 bit flags
    db 0 ;base 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
load32:
    mov eax, 1
    mov ecx, 100
    mov edi, 0x0100000 ;equivalent to 1 M
    call ata_lba_read
    jmp CODE_SEG:0x0100000

ata_lba_read:
    mov ebx, eax ;backup the lba
    ; send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0 ;select the master drive
    mov dx, 0x1F6
    out dx, al

    ;send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al

    ;restore the backup lba
    mov eax, ebx
    ;send more bits of the lba
    mov dx, 0x1F3
    out dx, al

    mov dx, 0x1F4
    mov eax, ebx ;restore the backup
    shr eax, 8
    out dx, al

    ;send upper 16 bits of the lba
    mov dx, 0x1F5
    mov eax, ebx ;restore the backup
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

    ;read all sectors into memory

.next_sector:
    push ecx

;check if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

    ; need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector

    ret

;boot signature
times 510-($ - $$) db 0 ; fill at least 510 bytes of data, pack the rest with 0s
dw 0xAA55
