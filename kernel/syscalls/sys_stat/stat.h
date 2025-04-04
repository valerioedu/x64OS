#ifndef STAT_H
#define STAT_H

#include "../../../lib/definitions.h"
#include "../../fs/src/file.h"

int stat(const char* pathname, struct stat* statbuf) {
    if (!pathname || !statbuf) {
        return -1;
    }

    int result = file_stat(pathname, statbuf);

    return (result == 0) ? 0 : -1;
}

int fstat(int fd, struct stat* statbuf) {
    if (!statbuf) {
        return -1;
    }

    if (fd < 3) {
        statbuf->st_mode = 0100644;
        statbuf->st_size = 0;
        statbuf->st_atime = 0;
        statbuf->st_mtime = 0;
        statbuf->st_ctime = 0;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        return 0;
    }

    int fs_fd = fd - 3;

    return file_fstat(fs_fd, statbuf);
}

#endif