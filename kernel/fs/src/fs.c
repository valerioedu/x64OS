#include "../fs.h"
#include "vfs.h"
#include "diskfs.h"

Inode* global_root = NULL;
static char current_path[256] = "/";

void fs_init() {
    SuperBlock* root_sb = diskfs_mount(0, 0, 1);  // Primary drive, first sector, auto-format
    if (!root_sb) {
        kprintf("Failed to mount root filesystem\n");
        return;
    }
    
    global_root = root_sb->root;
    current_directory = global_root;
    
    kprintf("Filesystem initialized with root at inode %d\n", 
            ((InodeCacheEntry*)global_root->fs_specific)->inode_num);
}

Inode* get_current_dir() {
    return current_directory;
}

Inode* get_root() {
    return global_root;
}

const char* get_current_path() {
    return current_path;
}

int change_dir(const char* path) {
    if (!path || !current_directory) return -1;
    
    if (strcmp(path, ".") == 0) {
        return 0;
    }
    
    if (strcmp(path, "..") == 0) {
        if (current_directory == global_root) {
            return 0;
        }
        
        Inode* parent = NULL;
        if (current_directory->ops->lookup(current_directory, "..", &parent)) {
            current_directory = parent;
            
            char* last_slash = strrchr(current_path, '/');
            if (last_slash != current_path) {
                *last_slash = '\0';
            } else {
                current_path[1] = '\0';
            }
            
            return 0;
        }
        return -1;
    }
    
    if (strcmp(path, "/") == 0) {
        current_directory = global_root;
        strcpy(current_path, "/");
        return 0;
    }
    
    Inode* target = NULL;
    if (!current_directory->ops->lookup(current_directory, path, &target)) {
        return -1;
    }
    
    InodeCacheEntry* target_ice = (InodeCacheEntry*)target->fs_specific;
    
    if (!(target_ice->inode.flags & INODE_DIRECTORY)) {
        kfree(target);
        return -1;
    }
    
    current_directory = target;
    
    if (current_path[strlen(current_path)-1] != '/') {
        strcat(current_path, "/");
    }
    strcat(current_path, path);
    
    return 0;
}