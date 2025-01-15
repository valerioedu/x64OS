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

    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    mov dh, 10
    mov dl, [BOOT_DRIVE]
    call disk_load

    ;mov bx, realbit_msg
    ;call bios_print

    call switch_to_pm

    jmp $

;%include "src/debug16.asm"
%include "src/disk.asm"
%include "src/stage1.asm"
%include "src/gdt32.asm"
;%include "src/print_hex.asm"

protbit_msg: db "Loaded 32 bit mode", 0
test1: db "test", 0
BOOT_DRIVE: db 0x80

[bits 32]
begin_pm:
    mov ax, DATA_SEG32
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x9000
    mov esp, ebp

    mov ebx, protbit_msg
    call print_pmode

    ;call delay_loop

    

    mov ebx, test1
    call print_pmode

    ;call delay_loop

    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosb
    mov edi, cr3

    mov DWORD [edi], 0x2003
    add edi, 0x1000
    mov DWORD [edi], 0x3003
    add edi, 0x1000
    mov DWORD [edi], 0x4003
    add edi, 0x1000

    mov ebx, 0x00000003
    mov ecx, 512

.SetEntry:
    mov DWORD [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .SetEntry

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.Pointer]
    jmp gdt64.Code:begin_lm

;delay_loop:
;    push ecx
;    mov ecx, 100000000  ; 1 million iterations
;.loop:
;    nop              ; Halt the CPU until the next interrupt
;    loop .loop       ; Decrement ECX and jump to .loop if ECX is not zero
;    pop ecx
;    ret

%include "src/debug32.asm"
%include "src/gdt64.asm"

[bits 64]
begin_lm:
    mov rsp, 0x9000
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax

    jmp $

times 510-($-$$) db 0
dw 0xAA55