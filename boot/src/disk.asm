disk_load:
    pusha 
    push dx     ; number of sectors (input parameter)

    mov ah, 0x02    ; read function 
    mov al, dh      ; number of sectors
    mov cl, 0x02    ; start reading from second sector (after boot sector)
    mov ch, 0x00    ; cylinder 0
    mov dh, 0x00    ; head 0

    ; dl should already contain the correct drive number from the caller
    ; read data to [es:bx] 
    int 0x13
    jc disk_error   ; carry bit is set -> error

    pop dx 
    cmp al, dh      ; read correct number of sectors
    jne sectors_error

    popa 
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call bios_print 
    mov dh, ah      ; ah = error code, dl = disk drive that dropped the error
    call print_hex  ; this will print the error code
    jmp disk_loop

sectors_error:
    mov bx, SECTORS_ERROR_MSG
    call bios_print

disk_loop:
    jmp $

DISK_ERROR_MSG: db "Disk read error, Error code: ", 0
SECTORS_ERROR_MSG: db "Incorrect number of sectors read", 0