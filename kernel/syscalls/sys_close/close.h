#ifndef CLOSE_H
#define CLOSE_H

#include "../../../lib/definitions.h"
#include "../../fs/src/file.h"

int close(int fd) {
    if (fd < 3) return -1;

    int fs_fd = fd - 3;

    return file_close(fs_fd);
}

#endif