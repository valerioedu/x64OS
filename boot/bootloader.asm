[org 0x7C00]
[bits 16]

_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov bp, 0x8000
    mov sp, bp

    mov [BOOT_DRIVE], dl

    mov bx, 0x1000
    mov dh, 2
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov bx, realbit_msg
    call bios_print

    call switch_to_pm

    jmp $

%include "src/debug16.asm"
%include "src/disk.asm"
%include "src/stage1.asm"
%include "src/gdt32.asm"
%include "src/print_hex.asm"

realbit_msg: db "About to load 32 bit mode", 0
protbit_msg: db "Loaded 32 bit mode", 0
test1: db "test", 0
BOOT_DRIVE: db 0

[bits 32]
begin_pm:
    mov ax, DATA_SEG32
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    mov ebx, protbit_msg
    call print_pmode

    ;call delay_loop

    

    mov ebx, test1
    call print_pmode

    call delay_loop

    jmp 0x1000

    jmp $

delay_loop:
    push ecx
    mov ecx, 100000000  ; 1 million iterations
.loop:
    nop              ; Halt the CPU until the next interrupt
    loop .loop       ; Decrement ECX and jump to .loop if ECX is not zero
    pop ecx
    ret

%include "src/debug32.asm"

times 510-($-$$) db 0
dw 0xAA55