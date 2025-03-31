#include "write.h"
#include "../../drivers/vga/vga.h"
#include "../../fs/src/file.h"

ssize_t write(int fd, const void* buf, size_t nbyte) {
    if (fd == stdout) {
        for (int i = 0; i < nbyte; i++) {
            vga_putc(((char*)buf)[i]);
        }
        return nbyte;
    }
    else if (fd == stderr) {
        for (int i = 0; i < nbyte; i++) {
            vga_putc(((char*)buf)[i]);
        }
        return nbyte;
    }
    else if (fd == stdin) return -1;
    else if (fd >= 3 && fd < MAX_OPEN_FILES + 3) {
        int fs_fd = fd - 3;
        return file_write(fs_fd, buf, nbyte);
    }
    return -1;
}