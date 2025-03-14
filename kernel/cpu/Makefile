CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

all: idt.o idt_load.o interrupts.o isr.o fpu.o

idt.o: src/idt.c
	$(CC) $(CFLAGS) $< -o $@

idt_load.o: src/idt_load.asm
	nasm -f elf64 -o $@ $<

interrupts.o: src/interrupts.asm
	nasm -f elf64 -o $@ $<

isr.o: src/isr.c
	$(CC) $(CFLAGS) $< -o $@

fpu.o: src/fpu.asm
	nasm -f elf64 -o $@ $<

clean:
	rm -f *.o