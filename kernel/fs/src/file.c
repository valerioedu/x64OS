#include "file.h"
#include "vfs.h"
#include "diskfs.h"
#include "../../mm/heap.h"

static FileDescriptor file_table[MAX_OPEN_FILES];

void file_init() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        file_table[i].inode = NULL;
        file_table[i].position = 0;
        file_table[i].flags = 0;
        file_table[i].refcount = 0;
    }
}

static int get_free_fd() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_table[i].refcount == 0) {
            return i;
        }
    }
    return -1;
}

int file_open(const char* path, int flags, int mode) {
    if (!path) return -1;
    
    int fd = get_free_fd();
    if (fd < 0) {
        return -1;
    }
    
    Inode* start_dir = NULL;
    if (path[0] == '/') {
        start_dir = root_inode;
    } else {
        start_dir = current_directory;
    }
    
    if (!start_dir) {
        return -1;
    }
    
    Inode* found_inode = NULL;
    int result = vfs_lookup(start_dir, path, &found_inode);
    
    if (!result && (flags & O_CREAT)) {
        char dir_path[256] = {0};
        char filename[64] = {0};
        
        const char* last_slash = strrchr(path, '/');
        
        if (last_slash) {
            strncpy(dir_path, path, last_slash - path);
            dir_path[last_slash - path] = '\0';
            
            strcpy(filename, last_slash + 1);
            
            Inode* parent_dir = NULL;
            if (dir_path[0] == '\0') {
                parent_dir = start_dir;
            } else {
                if (!vfs_lookup(start_dir, dir_path, &parent_dir) || !parent_dir) {
                    return -1;
                }
            }
            
            if (!parent_dir->ops->create(parent_dir, filename, mode)) {
                if (parent_dir != start_dir) {
                    kfree(parent_dir);
                }
                return -1;
            }
            
            result = vfs_lookup(parent_dir, filename, &found_inode);
            
            if (parent_dir != start_dir) {
                kfree(parent_dir);
            }
            
            if (!result || !found_inode) {
                return -1;
            }
        } else {
            if (!start_dir->ops->create(start_dir, path, mode)) {
                return -1;
            }
            
            result = vfs_lookup(start_dir, path, &found_inode);
            if (!result || !found_inode) {
                return -1;
            }
        }
    } else if (!result || !found_inode) {
        return -1;
    }
    
    file_table[fd].inode = found_inode;
    file_table[fd].position = 0;
    file_table[fd].flags = flags;
    file_table[fd].refcount = 1;
    
    if (flags & O_APPEND) {
        file_table[fd].position = found_inode->size;
    }
    
    if (flags & O_TRUNC) {
        if (file_table[fd].inode->ops->truncate) {
            if (!file_table[fd].inode->ops->truncate(file_table[fd].inode, 0)) {
                file_table[fd].inode = NULL;
                file_table[fd].refcount = 0;
                return -1;
            }
        } else {
            file_table[fd].inode->size = 0;
        }
    }
    
    return fd;
}

int file_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || file_table[fd].refcount == 0) {
        return -1;
    }
    
    file_table[fd].refcount--;
    
    if (file_table[fd].refcount == 0) {
        file_table[fd].inode = NULL;
        file_table[fd].position = 0;
        file_table[fd].flags = 0;
    }
    
    return 0;
}

int file_read(int fd, void* buffer, uint64_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || file_table[fd].refcount == 0 || !buffer) {
        return -1;
    }
    
    if (!(file_table[fd].flags & (O_RDONLY | O_RDWR))) {
        return -1;
    }
    
    Inode* inode = file_table[fd].inode;
    uint64_t position = file_table[fd].position;
    
    int bytes_read = inode->ops->read(inode, position, buffer, count);
    if (bytes_read > 0) {
        file_table[fd].position += bytes_read;
    }
    
    return bytes_read;
}

int file_write(int fd, const void* buffer, uint64_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || file_table[fd].refcount == 0 || !buffer) {
        return -1;
    }
    
    if (!(file_table[fd].flags & (O_WRONLY | O_RDWR))) {
        return -1;
    }
    
    Inode* inode = file_table[fd].inode;
    uint64_t position = file_table[fd].position;
    
    int bytes_written = inode->ops->write(inode, position, buffer, count);
    if (bytes_written > 0) {
        file_table[fd].position += bytes_written;
        
        if (file_table[fd].position > inode->size) {
            inode->size = file_table[fd].position;
        }
    }
    
    return bytes_written;
}

int file_seek(int fd, uint64_t offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || file_table[fd].refcount == 0) {
        return -1;
    }
    
    uint64_t new_pos = 0;
    
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file_table[fd].position + offset;
            break;
        case SEEK_END:
            new_pos = file_table[fd].inode->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < 0) {
        return -1;
    }
    
    file_table[fd].position = new_pos;
    return new_pos;
}

int file_stat(const char* path, struct stat* buf) {
    if (!path || !buf) {
        return -1;
    }
    
    Inode* start_dir = NULL;
    if (path[0] == '/') {
        start_dir = root_inode;
    } else {
        start_dir = current_directory;
    }
    
    if (!start_dir) {
        return -1;
    }
    
    Inode* inode = NULL;
    if (!vfs_lookup(start_dir, path, &inode) || !inode) {
        return -1;
    }
    
    buf->st_mode = inode->mode;
    buf->st_size = inode->size;
    
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;
    
    kfree(inode);    
    return 0;
}

int file_create(const char* path, int mode) {
    return file_open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int file_unlink(const char* path) {
    if (!path) {
        return -1;
    }
    
    Inode* start_dir = NULL;
    if (path[0] == '/') {
        start_dir = root_inode;
    } else {
        start_dir = current_directory;
    }
    
    if (!start_dir) {
        return -1;
    }
    
    char dir_path[256] = {0};
    char filename[64] = {0};
    
    const char* last_slash = strrchr(path, '/');
    
    if (last_slash) {
        strncpy(dir_path, path, last_slash - path);
        dir_path[last_slash - path] = '\0';
        
        strcpy(filename, last_slash + 1);
        
        Inode* parent_dir = NULL;
        if (dir_path[0] == '\0') {
            parent_dir = start_dir;
        } else {
            if (!vfs_lookup(start_dir, dir_path, &parent_dir) || !parent_dir) {
                return -1;
            }
        }
        
        int result = parent_dir->ops->delete(parent_dir, filename);
        
        if (parent_dir != start_dir) {
            kfree(parent_dir);
        }
        
        return result ? 0 : -1;
    } else {
        return start_dir->ops->delete(start_dir, path) ? 0 : -1;
    }
}

int file_fstat(int fd, struct stat* buf) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || file_table[fd].refcount == 0 || !buf) {
        return -1;
    }
    
    Inode* inode = file_table[fd].inode;
    
    buf->st_mode = inode->mode;
    buf->st_size = inode->size;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;
    
    return 0;
}