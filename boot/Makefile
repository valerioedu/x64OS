CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c
LDFLAGS = -m elf_x86_64 -T boot.ld --oformat binary

all: bootloader run clean

bootloader.bin: bootloader.asm
	$(AS) -f bin -o $@ $<

bootloader: cpu.bin bootloader.bin
	cat bootloader.bin cpu.bin > bootloader

cpu.bin: cpu.o paging.o
	$(LD) $(LDFLAGS) -o $@ $^

cpu.o: cpu.c cpu/cpu.h
	$(CC) $(CFLAGS) -o $@ $<

paging.o: cpu/paging.c cpu/cpu.h ../lib/definitions.h
	$(CC) $(CFLAGS) -o $@ $<

run:
	qemu-system-x86_64 -drive format=raw,file=bootloader

clean:
	rm -f *.o *.bin bootloader