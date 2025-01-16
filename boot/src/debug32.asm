[bits 32]

VIDEO_MEM equ 0xb8000 

print_pmode:
	pusha 
	mov edx, VIDEO_MEM

print_pmode_loop:
	mov al, [ebx]
	mov ah, 0x0f
	
	cmp al, 0 
	je print_pmode_end 

	mov [edx], ax
	add ebx, 1
	add edx, 2 
	
	jmp print_pmode_loop

print_pmode_end:
	popa 
	ret 
