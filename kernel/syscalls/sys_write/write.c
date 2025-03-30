#include "write.h"
#include "../../drivers/vga/vga.h"

ssize_t write(int fd, const void* buf, size_t nbyte) {
    if (fd == stdout) {
        for (int i = 0; i < nbyte; i++) {
            vga_putc(((char*)buf)[i]);
        }
        return nbyte;
    }
    return -1;
}