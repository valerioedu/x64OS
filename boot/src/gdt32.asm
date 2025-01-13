[bits 16]

gdt_start32:
    dq 0x0

.code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

.data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end32:

gdt_descriptor32:
    dw gdt_end32 - gdt_start32 -1
    dd gdt_start32

CODE_SEG32 equ gdt_start32.code - gdt_start32
DATA_SEG32 equ gdt_start32.data - gdt_start32
