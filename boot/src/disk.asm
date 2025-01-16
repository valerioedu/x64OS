disk_load:
    pusha 
    push dx

    mov ah, 0x02
    mov al, dh
    mov cl, 0x02
    mov ch, 0x00
    mov dh, 0x00

    ; Print drive number
    ;push dx
    ;mov bx, DRIVE_MSG
    ;call bios_print
    ;mov dx, 0
    ;mov dl, [esp]   ; Get the drive number from the stack
    ;call print_hex
    ;mov bx, NEWLINE
    ;call bios_print
    ;pop dx
    ; dl should already contain the correct drive number from the caller
    ; read data to [es:bx] 
    int 0x13
    jc disk_error

    pop dx 
    cmp al, dh
    jne sectors_error

    popa 
    ret

disk_error:
    ;mov bx, DISK_ERROR_MSG
    ;call bios_print 
    ;mov dl, ah
    ;xor dh, dh
    ;call print_hex  ; this will print the error code
    ;jmp disk_loop

sectors_error:
    ;mov bx, SECTORS_ERROR_MSG
    ;call bios_print

disk_loop:
    jmp $

DISK_ERROR_MSG: db "Disk read error, Error code: ", 0
SECTORS_ERROR_MSG: db "Incorrect number of sectors read", 0
DRIVE_MSG: db "Drive number: ", 0
NEWLINE: db 13, 10, 0