#ifndef VFS_H
#define VFS_H

#include "../../mm/heap.h"
#include "../../../lib/definitions.h"

struct Inode;
struct SuperBlock;

typedef struct File {
    struct Inode* inode;
    uint64_t offset;
    int flags;
} File;

typedef struct InodeOps {
    int (*create)(struct Inode* dir, const char* name, int mode);
    int (*lookup)(struct Inode* dir, const char* name, struct Inode** result);
    int (*read)(struct Inode* inode, uint64_t offset, void* buffer, uint64_t size);
    int (*write)(struct Inode* inode, uint64_t offset, const void* buffer, uint64_t size);
    int (*delete)(struct Inode* dir, const char* name);
    int (*mkdir)(struct Inode* dir, const char* name, int mode);
} InodeOps;

typedef struct Inode {
    uint32_t mode;
    uint32_t size;
    struct SuperBlock* sb;
    InodeOps* ops;
    void* fs_specific;
} Inode;

typedef struct SuperBlock {
    Inode* root;
    void* fs_specific;
} SuperBlock;

int   vfs_mount(const char* mountpoint, SuperBlock* sb);
File* vfs_open(const char* path, int flags);
int   vfs_read(File* file, void* buffer, uint64_t size);
int   vfs_write(File* file, const void* buffer, uint64_t size);
int   vfs_close(File* file);
int vfs_lookup(Inode* start_dir, const char* path, Inode** result);

#endif
