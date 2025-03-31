CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

all: context_switch.o queue.o scheduling.o

queue.o: src/queue.c
	$(CC) $(CFLAGS) $< -o $@

context_switch.o: src/context_switch.asm
	nasm -f elf64 -o $@ $<

scheduling.o: src/scheduling.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o