CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c

LD = x86_64-linux-gnu-ld

all: syscalls.o close.o open.o read.o sleep.o stat.o write.o syscalls.o

close.o: sys_close/close.c
	$(CC) $(CFLAGS) $< -o $@

open.o: sys_open/open.c
	$(CC) $(CFLAGS) $< -o $@

read.o: sys_read/read.c
	$(CC) $(CFLAGS) $< -o $@

sleep.o: sys_sleep/sleep.c
	$(CC) $(CFLAGS) $< -o $@

stat.o: sys_stat/stat.c
	$(CC) $(CFLAGS) $< -o $@

write.o: sys_write/write.c
	$(CC) $(CFLAGS) $< -o $@

sys.o: sys.c
	$(CC) $(CFLAGS) $< -o $@

syscalls.o: write.o read.o open.o close.o sleep.o stat.o sys.o
	$(LD) -r -o $@ $^

clean:
	rm -f *.o