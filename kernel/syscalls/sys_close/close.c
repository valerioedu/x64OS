#include "close.h"

int close(int fd) {
    if (fd < 3) return -1;

    int fs_fd = fd - 3;

    return file_close(fs_fd);
}