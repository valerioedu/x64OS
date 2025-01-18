global idt_load

section .text

idt_load:
    mov rax, [rdi]
    mov rdx, [rdi + 8]
    lidt [rdi]
    ret
