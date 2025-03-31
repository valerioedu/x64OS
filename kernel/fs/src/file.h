#ifndef _FILE_H
#define _FILE_H

#include "../../../lib/definitions.h"
#include "vfs.h"

#define MAX_OPEN_FILES 16

typedef struct {
    Inode* inode;
    uint64_t position;
    uint32_t flags;
    int refcount;
} FileDescriptor;

#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0003
#define O_CREAT     0x0100
#define O_APPEND    0x0008
#define O_TRUNC     0x0400

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat {
    uint32_t st_mode;
    uint64_t st_size;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_atime;
    uint64_t st_mtime;
    uint64_t st_ctime;
};

int file_open(const char* path, int flags, int mode);
int file_close(int fd);
int file_read(int fd, void* buffer, uint64_t count);
int file_write(int fd, const void* buffer, uint64_t count);
int file_seek(int fd, uint64_t offset, int whence);
int file_stat(const char* path, struct stat* buf);
int file_create(const char* path, int mode);
int file_unlink(const char* path);
int file_fstat(int fd, struct stat* buf);

#endif