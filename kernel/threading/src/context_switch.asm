global context_switch
section .text

;AMD64 calling convention:
;1st in rdi = old_task
;2nd in rsi = new_task

context_switch:
    cmp rdi, 0
    je .skip_old

.skip_old:
    cmp rsi, 0
    je .finish

    mov rax, [rsi + 8*5]
    test rax, rax
    jz .no_cr3
    mov cr3, rax

.no_cr3:
    lea rax, [rsi + <offset]