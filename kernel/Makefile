CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld

INCLUDE_PATHS = -I$(PWD) -I$(PWD)/.. -I$(PWD)/../lib
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib $(INCLUDE_PATHS) -c

all: submake vga.o kernel.o string.o heap.o cpu/idt.o cpu/idt_load.o keyboard.o ide.o macros.o input.o read.o

submake:
	$(MAKE) -C cpu
	$(MAKE) -C threading

vga.o: drivers/vga/vga.c
	$(CC) $(CFLAGS) $< -o $@ 

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) $< -o $@

heap.o: mm/src/heap.c
	$(CC) $(CFLAGS) $< -o $@

keyboard.o: drivers/keyboard/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

macros.o: drivers/keyboard/macros.c
	$(CC) $(CFLAGS) $< -o $@

ide.o: drivers/IDE/ide.c
	$(CC) $(CFLAGS) $< -o $@

string.o: ../lib/src/string.c
	$(CC) $(CFLAGS) $< -o $@

input.o: ../lib/src/input.c
	$(CC) $(CFLAGS) $< -o $@

read.o: syscalls/sys_read/read.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	$(MAKE) -C cpu clean
#	$(MAKE) -C threading clean