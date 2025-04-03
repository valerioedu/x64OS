CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

all: binary.o

binary.o: src/binary.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o