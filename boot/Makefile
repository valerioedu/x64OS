CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c
LDFLAGS = -m elf_x86_64 -T kernel.ld --oformat binary

DISK_SIZE = 16

all: submake disk.img run clean

submake:
	$(MAKE) -C ../kernel

bootloader.bin: bootloader.asm
	$(AS) -f bin -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) $< -o $@

main.bin: main.o ../kernel/vga.o ../kernel/string.o ../kernel/kernel.o \
		 ../kernel/heap.o ../kernel/cpu/idt.o ../kernel/cpu/idt_load.o \
		 ../kernel/cpu/interrupts.o ../kernel/cpu/isr.o ../kernel/keyboard.o \
		 ../kernel/cpu/fpu.o ../kernel/ide.o ../kernel/input.o \
		 ../kernel/fs/vfs.o ../kernel/fs/diskfs.o ../kernel/fs/fs.o \
		 ../kernel/fs/file.o ../shell/shell.o ../shell/rm.o ../shell/cd.o ../shell/ls.o \
		 ../shell/help.o ../shell/clear.o ../shell/touch.o ../shell/mkdir.o ../shell/exec.o \
		 ../kernel/threading/binary.o ../kernel/paging.o ../kernel/pci.o ../kernel/syscalls/syscalls.o #\
		 ../kernel/threading/context_switch.o ../kernel/threading/queue.o \
		 ../kernel/threading/scheduling.o 
	$(LD) $(LDFLAGS) $^ -o $@

disk.img: bootloader.bin main.bin
	dd if=/dev/zero of=$@ bs=1M count=16
	dd if=bootloader.bin of=$@ conv=notrunc
	dd if=main.bin of=$@ conv=notrunc seek=1

run:
	qemu-system-x86_64 -hda disk.img

clean:
	rm -f *.o *.bin disk.img
	$(MAKE) -C ../kernel clean