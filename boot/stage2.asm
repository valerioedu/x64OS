[bits 32]
[org 0x1000]

start:
    call setup_paging

    call switch_to_long_mode

    jmp $

delay_loop:
    push ecx
    mov ecx, 100000000  ; 1 million iterations
.loop:
    nop              ; Halt the CPU until the next interrupt
    loop .loop       ; Decrement ECX and jump to .loop if ECX is not zero
    pop ecx
    ret

%include "src/bit64.asm"
%include "src/debug32.asm"
%include "cpu/paging.asm"