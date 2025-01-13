bios_print:
    pusha

.loop:
    mov al, [bx]
    cmp al, 0
    je .end

    mov ah, 0x0E
    int 0x10

    add bx, 1
    jmp .loop

.end:
    popa
    ret

bios_newline:
    pusha

    mov ah, 0x0E
    mov al, 0x0A

    int 0x10

    popa
    ret
