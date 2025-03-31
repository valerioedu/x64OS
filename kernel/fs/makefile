CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

all: vfs.o diskfs.o fs.o file.o

vfs.o: src/vfs.c
	$(CC) $(CFLAGS) $< -o $@

diskfs.o: src/diskfs.c
	$(CC) $(CFLAGS) $< -o $@

fs.o: src/fs.c
	$(CC) $(CFLAGS) $< -o $@

file.o: src/file.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o