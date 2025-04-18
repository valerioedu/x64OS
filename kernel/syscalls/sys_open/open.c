#include "open.h"
#include "../../fs/src/file.h"

int open(const char* pathname, int flags, mode_t mode) {
    if (!pathname) {
        return -1;
    }

    int fs_fd = file_open(pathname, flags, mode);

    if (fs_fd < 0) {
        return -1;
    }

    int fd = fs_fd + 3;
    return fd;
}