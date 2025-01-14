; Prints the value of DX as hex.
print_hex:
    pusha
    mov cx, 4   ; 4 hex digits
hex_loop:
    dec cx
    mov ax, dx
    shr dx, 4
    and ax, 0xf
    mov bx, HEX_OUT
    add bx, 2
    add bx, cx
    cmp ax, 0xa
    jl set_letter
    add al, 0x27  ; 'A' is 65 and 'a' is 97, so add 27 to get lowercase
set_letter:
    add al, 0x30
    mov byte [bx], al
    cmp cx, 0
    jne hex_loop
    mov bx, HEX_OUT
    call bios_print
    popa
    ret

HEX_OUT: db '0x0000', 0