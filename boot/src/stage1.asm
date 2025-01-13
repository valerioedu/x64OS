[bits 16]
switch_to_pm:
    cli

    lgdt [gdt_descriptor32]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG32:init_pm

[bits 32]
init_pm:
    mov esp, 0x90000
    jmp begin_pm
