CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld

INCLUDE_PATHS = -I$(PWD) -I$(PWD)/.. -I$(PWD)/../lib
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib $(INCLUDE_PATHS) -c

all: vga.o kernel.o string.o heap.o

vga.o: drivers/vga/vga.c
	$(CC) $(CFLAGS) $< -o $@ 

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) $< -o $@

heap.o: mm/src/heap.c
	$(CC) $(CFLAGS) $< -o $@

string.o: ../lib/src/string.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm .f *.o