global fpu_init
global get_fpu_status

section .text

fpu_init:
    clts
    mov rax, cr0
    and rax, ~(1<<2)
    or  eax, (1<<1)
    mov cr0, rax

    mov rax, cr4
    or rax, (3<<9)
    mov cr4, rax

    fninit
    mov rax, 0
    ret

get_fpu_status:
    fnstsw ax
    mov rax, rax
    ret