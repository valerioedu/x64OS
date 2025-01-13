[bits 32]
switch_to_long_mode:
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    
    mov dword [0xb8000], 0x2f4b2f4f

    call delay_loop

    mov cr0, eax
        
    mov dword [0xb8000], 0x1F691F6E1F6F1F4C

    call delay_loop

    jmp long_mode_start

[bits 64]
long_mode_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rax, 0x1F201F201F201F20  ; White space on blue background
    mov rcx, 500
    mov rdi, 0xB8000
    rep stosq                    ; Clear the screen

    mov rax, 0x1F691F6E1F6F1F4C  ; "Long" in white on blue
    mov [0xB8000], rax
    mov rax, 0x1F6F1F641F651F20  ; " mode" in white on blue
    mov [0xB8008], rax

    ; Halt the CPU
    hlt
