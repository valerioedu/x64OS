CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

all: shell.o rm.o cd.o ls.o help.o clear.o touch.o mkdir.o exec.o

shell.o: shell.c
	$(CC) $(CFLAGS) $< -o $@

rm.o: src/rm.c
	$(CC) $(CFLAGS) $< -o $@

cd.o: src/cd.c
	$(CC) $(CFLAGS) $< -o $@

ls.o: src/ls.c
	$(CC) $(CFLAGS) $< -o $@

help.o: src/help.c
	$(CC) $(CFLAGS) $< -o $@

clear.o: src/clear.c
	$(CC) $(CFLAGS) $< -o $@

touch.o: src/touch.c
	$(CC) $(CFLAGS) $< -o $@

mkdir.o: src/mkdir.c
	$(CC) $(CFLAGS) $< -o $@

exec.o: src/exe.c
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f *.o