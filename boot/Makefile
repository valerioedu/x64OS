CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c
LDFLAGS = -m elf_x86_64 -T boot.ld --oformat binary

DISK_SIZE = 16

all: disk.img run clean

bootloader.bin: bootloader.asm
	$(AS) -f bin -o $@ $<

disk.img: bootloader.bin
	dd if=/dev/zero of=$@ bs=1M count=16
	dd if=bootloader.bin of=$@ conv=notrunc

run:
	qemu-system-x86_64 -hda disk.img

clean:
	rm -f *.o *.bin bootloader