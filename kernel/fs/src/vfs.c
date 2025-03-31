#include "vfs.h"
#include "diskfs.h"
#include "file.h"
#include "../../mm/heap.h"
#include "../../drivers/vga/vga.h"
#include "../../../lib/definitions.h"

static SuperBlock* g_root_sb = NULL;

static Inode* find_inode_by_path(const char* path) {
    if (!g_root_sb) return NULL;

    Inode* root = g_root_sb->root;
    if (!root) return NULL;

    if (path[0] == '/' && path[1] == '\0') {
        return root;
    }

    const char* name = (path[0] == '/') ? path + 1 : path;
    if (root->ops && root->ops->lookup) {
        Inode* result = NULL;
        if (root->ops->lookup(root, name, &result) == 0) {
            return result;
        }
    }
    return NULL;
}

int vfs_mount(const char* mountpoint, SuperBlock* sb) {
    (void)mountpoint;
    g_root_sb = sb;
    return 0;
}

File* vfs_open(const char* path, int flags) {
    Inode* inode = find_inode_by_path(path);
    if (!inode && (flags & O_CREAT)) {
        char dir_path[256] = {0};
        char filename[64] = {0};
        
        const char* last_slash = strrchr(path, '/');
        
        if (last_slash) {
            strncpy(dir_path, path, last_slash - path);
            dir_path[last_slash - path] = '\0';
            
            strcpy(filename, last_slash + 1);
            
            Inode* parent_dir = find_inode_by_path(dir_path[0] ? dir_path : "/");
            if (!parent_dir) {
                return NULL;
            }
            
            if (!parent_dir->ops->create(parent_dir, filename, 0644)) {
                return NULL;
            }
            
            inode = find_inode_by_path(path);
            if (!inode) {
                return NULL;
            }
        } else {
            Inode* root_dir = find_inode_by_path("/");
            if (!root_dir->ops->create(root_dir, path, 0644)) {
                return NULL;
            }
            
            inode = find_inode_by_path(path);
            if (!inode) {
                return NULL;
            }
        }
    } else if (!inode) {
        return NULL;
    }

    File* file = (File*)kmalloc(sizeof(File));
    if (!file) return NULL;

    file->inode  = inode;
    file->offset = 0;
    file->flags  = flags;
    return file;
}

int vfs_read(File* file, void* buffer, uint64_t size) {
    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->read) {
        return -1;
    }
    int bytes_read = file->inode->ops->read(file->inode, file->offset, buffer, size);
    if (bytes_read > 0) {
        file->offset += bytes_read;
    }
    return bytes_read;
}

int vfs_write(File* file, const void* buffer, uint64_t size) {
    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->write) {
        return -1;
    }
    int bytes_written = file->inode->ops->write(file->inode, file->offset, buffer, size);
    if (bytes_written > 0) {
        file->offset += bytes_written;
    }
    return bytes_written;
}

int vfs_close(File* file) {
    if (!file) return -1;
    kfree(file);
    return 0;
}

int vfs_lookup(Inode* start_dir, const char* path, Inode** result) {
    if (!start_dir || !path || !result) {
        return 0;
    }
    
    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';
    
    if (path_copy[0] == '/') {
        path = path + 1;
        
        if (path[0] == '\0') {
            *result = root_inode;
            return 1;
        }
    }
    
    Inode* current = start_dir;
    
    char* component = strtok(path_copy, "/");
    while (component) {
        if (strcmp(component, ".") == 0) {
        }
        else if (strcmp(component, "..") == 0) {
            if (current != root_inode) {
                Inode* parent = NULL;
                if (!current->ops->lookup(current, "..", &parent)) {
                    return 0;
                }
                
                if (current != start_dir) {
                    kfree(current);
                }
                
                current = parent;
            }
        }
        else {
            Inode* next = NULL;
            if (!current->ops->lookup(current, component, &next)) {
                return 0;
            }
            
            if (current != start_dir) {
                kfree(current);
            }
            
            current = next;
        }
        
        component = strtok(NULL, "/");
    }
    
    *result = current;
    return 1;
}