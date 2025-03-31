#ifndef FS_H
#define FS_H

#include "src/vfs.h"
#include "src/diskfs.h"
#include "src/file.h"

void fs_init();

Inode* get_current_dir();

Inode* get_root();

const char* get_current_path();

int change_dir(const char* path);

#endif