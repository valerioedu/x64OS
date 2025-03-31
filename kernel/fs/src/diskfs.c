#include "diskfs.h"
#include "vfs.h"
#include "../../mm/heap.h"
#include "../../drivers/vga/vga.h"
#include "../../../lib/definitions.h"
#include "../../cpu/interrupts.h"
#include "../../drivers/IDE/ide.h"
#include "../../cpu/src/pic.h"

void sleep(uint32_t ms) {
    uint64_t start = timer_get_ticks();
    uint64_t end = start + ms;
    
    while (timer_get_ticks() < end) {
        asm volatile ("nop");
    }
}

Inode* root_inode = NULL;
Inode* current_directory = NULL;
static uint16_t aligned_buffer[DISK_SECTOR_SIZE/2];

static int            diskfs_create_node(Inode* dir, const char* name, int mode);
static int            diskfs_lookup(Inode* dir, const char* name, Inode** result);
static int            diskfs_read(Inode* inode, uint64_t offset, void* buffer, uint64_t size);
static int            diskfs_write(Inode* inode, uint64_t offset, const void* buffer, uint64_t size);
static int            diskfs_mkdir(Inode* dir, const char* name, int mode);
static int            diskfs_delete(Inode* dir, const char* name);

static uint32_t        allocate_block(DiskfsInfo* dfs);
static void            free_block(DiskfsInfo* dfs, uint32_t block);
static InodeCacheEntry* get_inode(DiskfsInfo* dfs, uint32_t inode_num);
BlockCacheEntry* get_block(DiskfsInfo* dfs, uint32_t block_num);
static int             flush_inode(DiskfsInfo* dfs, InodeCacheEntry* ice);
static int             flush_block(DiskfsInfo* dfs, BlockCacheEntry* bce);
static int             init_filesystem(DiskfsInfo* dfs);
static int             init_directory(DiskfsInfo* dfs, uint32_t dir_block, uint32_t parent_inode);
static int             add_dir_entry(DiskfsInfo* dfs, uint32_t dir_inode, const char* name, uint32_t inode_num, uint8_t type);
static int             remove_dir_entry(DiskfsInfo* dfs, uint32_t dir_inode, const char* name);
static uint32_t        get_block_for_offset(DiskfsInfo* dfs, InodeCacheEntry* inode, uint64_t offset, bool allocate);
static void            release_inode(InodeCacheEntry* ice);
void            release_block(BlockCacheEntry* bce);
static int             parse_path(const char* path, char** components, int max_components);
static int             resolve_path(DiskfsInfo* dfs, const char* path, Inode** result);
static int diskfs_write_sector(uint8_t drive, uint32_t lba, const void* buffer);
static int journal_start_transaction(DiskfsInfo* dfs);
static int journal_log_block(DiskfsInfo* dfs, uint32_t block_num);
static int journal_commit_transaction(DiskfsInfo* dfs);
static void flush_all_cache(DiskfsInfo* dfs);
static int diskfs_truncate(Inode* inode, uint64_t size);

static InodeOps g_diskfs_inode_ops = {
    .create = diskfs_create_node,
    .lookup = diskfs_lookup,
    .read   = diskfs_read,
    .write  = diskfs_write,
    .delete = diskfs_delete,
    .mkdir  = diskfs_mkdir,
    .truncate = diskfs_truncate,
};

static Inode* create_vfs_inode(DiskfsInfo* dfs, InodeCacheEntry* ice) {
    if (!dfs || !ice) return NULL;
    
    Inode* inode = kmalloc(sizeof(Inode));
    if (!inode) return NULL;
    
    memset(inode, 0, sizeof(Inode));
    inode->mode = ice->inode.mode;
    inode->size = ice->inode.size;
    inode->ops = &g_diskfs_inode_ops;
    inode->fs_specific = ice;
    
    return inode;
}

static int diskfs_create_node(Inode* dir, const char* name, int mode) {
    if (!dir || !name || strlen(name) >= DISKFS_MAX_NAME) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)dir->sb->fs_specific;
    InodeCacheEntry* dir_ice = (InodeCacheEntry*)dir->fs_specific;
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("diskfs_create_node: Not a directory\n");
        return 0;
    }
    
    uint32_t inode_num = 0;
    for (uint32_t i = 0; i < dfs->super.total_inodes; i++) {
        InodeCacheEntry* ice = get_inode(dfs, i);
        if (ice && ice->inode.links == 0) {
            inode_num = i;
            release_inode(ice);
            break;
        }
        if (ice) release_inode(ice);
    }
    
    if (inode_num == 0) {
        kprintf("diskfs_create_node: No free inodes available\n");
        return 0;
    }
    
    InodeCacheEntry* new_ice = get_inode(dfs, inode_num);
    if (!new_ice) {
        kprintf("diskfs_create_node: Failed to get new inode\n");
        return 0;
    }
    
    memset(&new_ice->inode, 0, sizeof(DiskfsInode));
    new_ice->inode.mode = mode;
    new_ice->inode.uid = 0;
    new_ice->inode.gid = 0;
    new_ice->inode.size = 0;
    new_ice->inode.access_time = 0;
    new_ice->inode.modify_time = 0;
    new_ice->inode.create_time = 0;
    new_ice->inode.links = 1;
    new_ice->inode.flags = INODE_FILE;
    new_ice->dirty = 1;
    
    if (!flush_inode(dfs, new_ice)) {
        kprintf("diskfs_create_node: Failed to write new inode\n");
        release_inode(new_ice);
        return 0;
    }
    
    if (!add_dir_entry(dfs, dir_ice->inode_num, name, inode_num, INODE_FILE)) {
        kprintf("diskfs_create_node: Failed to add directory entry\n");
        
        new_ice->inode.links = 0;
        new_ice->dirty = 1;
        flush_inode(dfs, new_ice);
        release_inode(new_ice);
        return 0;
    }
    
    dfs->super.inode_count++;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf);
    
    release_inode(new_ice);
    return 1;
}

static int diskfs_lookup(Inode* dir, const char* name, Inode** result) {
    if (!dir || !name || !result) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)dir->sb->fs_specific;
    InodeCacheEntry* dir_ice = (InodeCacheEntry*)dir->fs_specific;
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("diskfs_lookup: Not a directory\n");
        return 0;
    }
    
    for (uint32_t block_idx = 0; block_idx < DIRECT_BLOCKS; block_idx++) {
        uint32_t block_num = dir_ice->inode.direct[block_idx];
        if (block_num == 0) continue;
        
        BlockCacheEntry* bce = get_block(dfs, block_num);
        if (!bce) continue;
        
        DiskfsDirectory* dir_header = (DiskfsDirectory*)bce->data;
        DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));
        
        for (uint32_t i = 0; i < dir_header->entry_count; i++) {
            if (strcmp(entries[i].name, name) == 0) {
                uint32_t inode_num = entries[i].inode;
                InodeCacheEntry* found_ice = get_inode(dfs, inode_num);
                if (!found_ice) {
                    release_block(bce);
                    return 0;
                }
                
                Inode* found_inode = create_vfs_inode(dfs, found_ice);
                if (!found_inode) {
                    release_inode(found_ice);
                    release_block(bce);
                    return 0;
                }
                
                found_inode->sb = dir->sb;
                *result = found_inode;
                
                release_block(bce);
                return 1;
            }
        }
        
        uint32_t next_block = dir_header->next_block;
        release_block(bce);
        
        if (next_block != 0) {
            while (next_block != 0) {
                BlockCacheEntry* next_bce = get_block(dfs, next_block);
                if (!next_bce) break;
                
                DiskfsDirectory* next_dir = (DiskfsDirectory*)next_bce->data;
                DiskfsDirEntry* next_entries = (DiskfsDirEntry*)(next_bce->data + sizeof(DiskfsDirectory));
                
                for (uint32_t i = 0; i < next_dir->entry_count; i++) {
                    if (strcmp(next_entries[i].name, name) == 0) {
                        uint32_t inode_num = next_entries[i].inode;
                        InodeCacheEntry* found_ice = get_inode(dfs, inode_num);
                        if (!found_ice) {
                            release_block(next_bce);
                            return 0;
                        }
                        
                        Inode* found_inode = create_vfs_inode(dfs, found_ice);
                        if (!found_inode) {
                            release_inode(found_ice);
                            release_block(next_bce);
                            return 0;
                        }
                        
                        found_inode->sb = dir->sb;
                        *result = found_inode;
                        
                        release_block(next_bce);
                        return 1;
                    }
                }
                
                uint32_t tmp_next = next_dir->next_block;
                release_block(next_bce);
                next_block = tmp_next;
            }
        }
    }
    
    return 0;
}

static uint32_t get_block_for_offset(DiskfsInfo* dfs, InodeCacheEntry* ice, uint64_t offset, bool allocate) {
    if (!dfs || !ice) return 0;
    
    uint32_t block_size = dfs->super.block_size;
    uint32_t block_index = offset / block_size;
    
    if (block_index < DIRECT_BLOCKS) {
        if (ice->inode.direct[block_index] == 0 && allocate) {
            uint32_t new_block = allocate_block(dfs);
            if (new_block == 0) return 0;
            
            ice->inode.direct[block_index] = new_block;
            ice->dirty = 1;
        }
        return ice->inode.direct[block_index];
    }
    
    block_index -= DIRECT_BLOCKS;
    uint32_t blocks_per_indirect = block_size / sizeof(uint32_t);
    
    if (block_index < blocks_per_indirect) {
        if (ice->inode.indirect == 0 && allocate) {
            uint32_t new_block = allocate_block(dfs);
            if (new_block == 0) return 0;
            
            ice->inode.indirect = new_block;
            ice->dirty = 1;
            
            BlockCacheEntry* bce = get_block(dfs, new_block);
            if (!bce) {
                free_block(dfs, new_block);
                ice->inode.indirect = 0;
                ice->dirty = 1;
                return 0;
            }
            
            memset(bce->data, 0, block_size);
            bce->dirty = 1;
            flush_block(dfs, bce);
            release_block(bce);
        }
        
        if (ice->inode.indirect == 0) return 0;
        
        BlockCacheEntry* bce = get_block(dfs, ice->inode.indirect);
        if (!bce) return 0;
        
        uint32_t* block_pointers = (uint32_t*)bce->data;
        uint32_t block_num = block_pointers[block_index];
        
        if (block_num == 0 && allocate) {
            uint32_t new_block = allocate_block(dfs);
            if (new_block == 0) {
                release_block(bce);
                return 0;
            }
            
            block_pointers[block_index] = new_block;
            bce->dirty = 1;
            flush_block(dfs, bce);
            release_block(bce);
            return new_block;
        }
        
        release_block(bce);
        return block_num;
    }
    
    // Double indirect blocks (not fully implemented)
    // For simplicity in this implementation
    return 0;
}

static int diskfs_read(Inode* inode, uint64_t offset, void* buffer, uint64_t size) {
    if (!inode || !buffer || size == 0) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)inode->sb->fs_specific;
    InodeCacheEntry* ice = (InodeCacheEntry*)inode->fs_specific;
    
    if (offset >= ice->inode.size) {
        return 0;
    }
    
    if (offset + size > ice->inode.size) {
        size = ice->inode.size - offset;
    }
    
    uint32_t block_size = dfs->super.block_size;
    uint32_t bytes_read = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    
    while (bytes_read < size) {
        uint32_t block_num = get_block_for_offset(dfs, ice, offset + bytes_read, false);
        if (block_num == 0) break;
        
        uint32_t offset_in_block = (offset + bytes_read) % block_size;
        uint32_t bytes_to_read = block_size - offset_in_block;
        
        if (bytes_to_read > size - bytes_read) {
            bytes_to_read = size - bytes_read;
        }
        
        BlockCacheEntry* bce = get_block(dfs, block_num);
        if (!bce) break;
        
        memcpy(buf_ptr + bytes_read, bce->data + offset_in_block, bytes_to_read);
        release_block(bce);
        
        bytes_read += bytes_to_read;
    }
    
    ice->inode.access_time = 0; // Should be current time
    ice->dirty = 1;
    flush_inode(dfs, ice);
    
    return bytes_read;
}

static int diskfs_write(Inode* inode, uint64_t offset, const void* buffer, uint64_t size) {
    if (!inode || !buffer || size == 0) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)inode->sb->fs_specific;
    InodeCacheEntry* ice = (InodeCacheEntry*)inode->fs_specific;
    
    uint32_t block_size = dfs->super.block_size;
    uint32_t bytes_written = 0;
    const uint8_t* buf_ptr = (const uint8_t*)buffer;
    
    if (dfs->super.journal_start != 0) {
        journal_start_transaction(dfs);
    }
    
    while (bytes_written < size) {
        uint32_t block_num = get_block_for_offset(dfs, ice, offset + bytes_written, true);
        if (block_num == 0) break;
        
        if (dfs->super.journal_start != 0) {
            journal_log_block(dfs, block_num);
        }
        
        uint32_t offset_in_block = (offset + bytes_written) % block_size;
        uint32_t bytes_to_write = block_size - offset_in_block;
        
        if (bytes_to_write > size - bytes_written) {
            bytes_to_write = size - bytes_written;
        }
        
        BlockCacheEntry* bce = get_block(dfs, block_num);
        if (!bce) break;
        
        memcpy(bce->data + offset_in_block, buf_ptr + bytes_written, bytes_to_write);
        bce->dirty = 1;
        flush_block(dfs, bce);
        release_block(bce);
        
        bytes_written += bytes_to_write;
    }
    
    if (offset + bytes_written > ice->inode.size) {
        ice->inode.size = offset + bytes_written;
        inode->size = ice->inode.size;
    }
    
    ice->inode.modify_time = 0;
    ice->dirty = 1;
    flush_inode(dfs, ice);
    
    if (dfs->super.journal_start != 0) {
        journal_commit_transaction(dfs);
    }
    
    return bytes_written;
}

//sleep to give time for the disk to finish writing
static int diskfs_mkdir(Inode* dir, const char* name, int mode) {
    if (!dir || !name || strlen(name) >= DISKFS_MAX_NAME) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)dir->sb->fs_specific;
    InodeCacheEntry* dir_ice = (InodeCacheEntry*)dir->fs_specific;
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("diskfs_mkdir: Not a directory\n");
        return 0;
    }
    
    uint32_t inode_num = 0;
    for (uint32_t i = 0; i < dfs->super.total_inodes; i++) {
        InodeCacheEntry* ice = get_inode(dfs, i);
        if (ice && ice->inode.links == 0) {
            inode_num = i;
            release_inode(ice);
            break;
        }
        if (ice) release_inode(ice);
    }
    
    if (inode_num == 0) {
        kprintf("diskfs_mkdir: No free inodes available\n");
        return 0;
    }
    
    sleep(5);
    
    if (dfs->super.journal_start != 0) {
        journal_start_transaction(dfs);
    }
    
    uint32_t dir_block = allocate_block(dfs);
    if (dir_block == 0) {
        kprintf("diskfs_mkdir: Failed to allocate directory block\n");
        return 0;
    }
    
    sleep(5);
    
    if (!init_directory(dfs, dir_block, dir_ice->inode_num)) {
        kprintf("diskfs_mkdir: Failed to initialize directory block\n");
        free_block(dfs, dir_block);
        return 0;
    }
    
    InodeCacheEntry* new_ice = get_inode(dfs, inode_num);
    if (!new_ice) {
        kprintf("diskfs_mkdir: Failed to get new inode\n");
        free_block(dfs, dir_block);
        return 0;
    }
    
    memset(&new_ice->inode, 0, sizeof(DiskfsInode));
    new_ice->inode.mode = mode | 0040000; // S_IFDIR in Unix
    new_ice->inode.uid = 0;
    new_ice->inode.gid = 0;
    new_ice->inode.size = sizeof(DiskfsDirectory);
    new_ice->inode.access_time = 0;
    new_ice->inode.modify_time = 0;
    new_ice->inode.create_time = 0;
    new_ice->inode.links = 1;
    new_ice->inode.flags = INODE_DIRECTORY;
    new_ice->inode.direct[0] = dir_block;
    new_ice->dirty = 1;
    
    if (!flush_inode(dfs, new_ice)) {
        kprintf("diskfs_mkdir: Failed to write new inode\n");
        release_inode(new_ice);
        free_block(dfs, dir_block);
        if (dfs->super.journal_start != 0) {
            journal_commit_transaction(dfs);
        }
        return 0;
    }
    
    sleep(5);
    
    if (!add_dir_entry(dfs, dir_ice->inode_num, name, inode_num, INODE_DIRECTORY)) {
        kprintf("diskfs_mkdir: Failed to add directory entry\n");
        
        new_ice->inode.links = 0;
        new_ice->dirty = 1;
        flush_inode(dfs, new_ice);
        release_inode(new_ice);
        free_block(dfs, dir_block);
        if (dfs->super.journal_start != 0) {
            journal_commit_transaction(dfs);
        }
        return 0;
    }
    
    sleep(5);
    
    if (!add_dir_entry(dfs, inode_num, ".", inode_num, INODE_DIRECTORY)) {
        kprintf("diskfs_mkdir: Failed to add '.' entry\n");
        remove_dir_entry(dfs, dir_ice->inode_num, name);
        new_ice->inode.links = 0;
        new_ice->dirty = 1;
        flush_inode(dfs, new_ice);
        release_inode(new_ice);
        free_block(dfs, dir_block);
        if (dfs->super.journal_start != 0) {
            journal_commit_transaction(dfs);
        }
        return 0;
    }
    
    sleep(5);
    
    if (!add_dir_entry(dfs, inode_num, "..", dir_ice->inode_num, INODE_DIRECTORY)) {
        kprintf("diskfs_mkdir: Failed to add '..' entry\n");
        remove_dir_entry(dfs, dir_ice->inode_num, name);
        remove_dir_entry(dfs, inode_num, ".");
        new_ice->inode.links = 0;
        new_ice->dirty = 1;
        flush_inode(dfs, new_ice);
        release_inode(new_ice);
        free_block(dfs, dir_block);
        if (dfs->super.journal_start != 0) {
            journal_commit_transaction(dfs);
        }
        return 0;
    }
    
    sleep(5);
    
    dir_ice->inode.links++;
    dir_ice->dirty = 1;
    flush_inode(dfs, dir_ice);
    
    dfs->super.inode_count++;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
        kprintf("diskfs_mkdir: Failed to update superblock\n");
    }
    
    flush_all_cache(dfs);
    
    if (dfs->super.journal_start != 0) {
        journal_commit_transaction(dfs);
    }
    
    release_inode(new_ice);
    return 1;
}

static int diskfs_delete(Inode* dir, const char* name) {
    if (!dir || !name) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)dir->sb->fs_specific;
    InodeCacheEntry* dir_ice = (InodeCacheEntry*)dir->fs_specific;
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("diskfs_delete: Not a directory\n");
        return 0;
    }
    
    Inode* target = NULL;
    if (!diskfs_lookup(dir, name, &target) || !target) {
        kprintf("diskfs_delete: File not found\n");
        return 0;
    }
    
    InodeCacheEntry* target_ice = (InodeCacheEntry*)target->fs_specific;
    uint32_t target_inode_num = target_ice->inode_num;
    
    if (target_ice->inode.flags & INODE_DIRECTORY) {
        BlockCacheEntry* bce = get_block(dfs, target_ice->inode.direct[0]);
        if (!bce) {
            kprintf("diskfs_delete: Failed to read directory block\n");
            kfree(target);
            return 0;
        }
        
        DiskfsDirectory* dir_header = (DiskfsDirectory*)bce->data;
        DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));
        
        int real_entries = 0;
        for (uint32_t i = 0; i < dir_header->entry_count; i++) {
            if (strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0) {
                real_entries++;
            }
        }
        
        release_block(bce);
        
        if (real_entries > 0) {
            kprintf("diskfs_delete: Directory not empty\n");
            kfree(target);
            return 0;
        }
        
        InodeCacheEntry* parent_ice = get_inode(dfs, dir_header->parent_inode);
        if (parent_ice) {
            parent_ice->inode.links--;
            parent_ice->dirty = 1;
            flush_inode(dfs, parent_ice);
            release_inode(parent_ice);
        }
    }
    
    if (!remove_dir_entry(dfs, dir_ice->inode_num, name)) {
        kprintf("diskfs_delete: Failed to remove directory entry\n");
        kfree(target);
        return 0;
    }
    
    for (uint32_t i = 0; i < DIRECT_BLOCKS; i++) {
        if (target_ice->inode.direct[i] != 0) {
            free_block(dfs, target_ice->inode.direct[i]);
            target_ice->inode.direct[i] = 0;
        }
    }
    
    if (target_ice->inode.indirect != 0) {
        BlockCacheEntry* indirect_bce = get_block(dfs, target_ice->inode.indirect);
        if (indirect_bce) {
            uint32_t* block_ptrs = (uint32_t*)indirect_bce->data;
            uint32_t blocks_per_indirect = dfs->super.block_size / sizeof(uint32_t);
            
            for (uint32_t i = 0; i < blocks_per_indirect; i++) {
                if (block_ptrs[i] != 0) {
                    free_block(dfs, block_ptrs[i]);
                }
            }
            
            release_block(indirect_bce);
            free_block(dfs, target_ice->inode.indirect);
            target_ice->inode.indirect = 0;
        }
    }
    
    target_ice->inode.links = 0;
    target_ice->inode.flags = 0;
    target_ice->inode.size = 0;
    target_ice->dirty = 1;
    flush_inode(dfs, target_ice);
    
    dfs->super.inode_count--;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf);
    
    kfree(target);
    return 1;
}

SuperBlock* diskfs_mount(uint8_t drive, uint32_t start_block, int auto_format) {
    DiskfsInfo* dfs = kmalloc(sizeof(DiskfsInfo));
    if (!dfs) {
        kprintf("diskfs_mount: Failed to allocate DiskfsInfo\n");
        return NULL;
    }
    
    memset(dfs, 0, sizeof(DiskfsInfo));
    dfs->drive = drive;
    dfs->start_block = start_block;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    if (!diskfs_read_sector(drive, start_block, sb_buf)) {
        kprintf("diskfs_mount: Failed to read superblock\n");
        kfree(dfs);
        return NULL;
    }
    
    memcpy(&dfs->super, sb_buf, sizeof(DiskfsSuper));
    
    if (dfs->super.magic != DISKFS_MAGIC) {
        kprintf("diskfs_mount: Invalid superblock magic (got %x, expected %x)\n", 
                dfs->super.magic, DISKFS_MAGIC);
        
        if (auto_format) {
            kprintf("Formatting disk for first use...\n");
            
            if (init_filesystem(dfs)) {
                kprintf("Disk formatted successfully.\n");
            } else {
                kprintf("Format failed.\n");
                kfree(dfs);
                return NULL;
            }
        } else {
            kprintf("Disk not formatted and auto-format disabled.\n");
            kfree(dfs);
            return NULL;
        }
    }
    
    InodeCacheEntry* root_ice = get_inode(dfs, dfs->super.root_inode);
    if (!root_ice) {
        kprintf("diskfs_mount: Failed to read root inode\n");
        kfree(dfs);
        return NULL;
    }
    
    SuperBlock* sb = kmalloc(sizeof(SuperBlock));
    if (!sb) {
        kprintf("diskfs_mount: Failed to allocate SuperBlock\n");
        kfree(dfs);
        return NULL;
    }
    
    memset(sb, 0, sizeof(SuperBlock));
    sb->fs_specific = dfs;
    
    Inode* root = create_vfs_inode(dfs, root_ice);
    if (!root) {
        kprintf("diskfs_mount: Failed to create root inode\n");
        kfree(sb);
        kfree(dfs);
        return NULL;
    }
    
    root->sb = sb;
    sb->root = root;
    
    root_inode = root;
    current_directory = root;
    
    kprintf("diskfs_mount: Filesystem mounted successfully\n");
    return sb;
}

int diskfs_read_sector(uint8_t drive, uint32_t lba, void* buffer) {
    if (!buffer) return 0;
    
    int result = ide_read(drive, lba, 1, aligned_buffer);
    if (result) {
        memcpy(buffer, aligned_buffer, DISK_SECTOR_SIZE);
    }
    return result;
}

static int diskfs_write_sector(uint8_t drive, uint32_t lba, const void* buffer) {
    if (!buffer) return 0;
    
    memcpy(aligned_buffer, buffer, DISK_SECTOR_SIZE);
    return ide_write(drive, lba, 1, aligned_buffer);
}

static inline void set_bitmap_bit(uint8_t* bitmap, uint32_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void clear_bitmap_bit(uint8_t* bitmap, uint32_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int test_bitmap_bit(uint8_t* bitmap, uint32_t bit) {
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

static uint32_t allocate_block(DiskfsInfo* dfs) {
    if (!dfs) return 0;
    
    if (dfs->super.free_blocks == 0) {
        kprintf("allocate_block: No free blocks available\n");
        return 0;
    }
    
    uint32_t bitmap_start_block = 2;
    
    uint32_t data_start_offset = dfs->super.data_blocks_start;
    for (uint32_t block = data_start_offset; block < dfs->super.total_blocks; block++) {
        uint32_t bit_offset = block - data_start_offset;
        uint32_t sector_offset = bit_offset / (DISK_SECTOR_SIZE * 8);
        uint32_t bit_in_sector = bit_offset % (DISK_SECTOR_SIZE * 8);
        
        uint8_t bitmap_sector[DISK_SECTOR_SIZE];
        if (!diskfs_read_sector(dfs->drive, dfs->start_block + bitmap_start_block + sector_offset, bitmap_sector)) {
            kprintf("allocate_block: Failed to read bitmap sector %d\n", bitmap_start_block + sector_offset);
            return 0;
        }
        
        if (!test_bitmap_bit(bitmap_sector, bit_in_sector)) {
            set_bitmap_bit(bitmap_sector, bit_in_sector);
            
            if (!diskfs_write_sector(dfs->drive, dfs->start_block + bitmap_start_block + sector_offset, bitmap_sector)) {
                kprintf("allocate_block: Failed to write bitmap sector %d\n", bitmap_start_block + sector_offset);
                return 0;
            }
            
            dfs->super.free_blocks--;
            uint8_t sb_buf[DISK_SECTOR_SIZE];
            memset(sb_buf, 0, DISK_SECTOR_SIZE);
            memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
            if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
                kprintf("allocate_block: Failed to update superblock\n");
                return 0;
            }
            
            sleep(5);
            
            uint8_t zero_buf[DISK_SECTOR_SIZE];
            memset(zero_buf, 0, DISK_SECTOR_SIZE);
            if (!diskfs_write_sector(dfs->drive, block, zero_buf)) {
                kprintf("allocate_block: Failed to initialize block %d\n", block);
                return 0;
            }
            
            return block;
        }
    }
    
    kprintf("allocate_block: No free blocks found\n");
    return 0;
}

static void free_block(DiskfsInfo* dfs, uint32_t block) {
    if (!dfs || block < dfs->super.data_blocks_start || block >= dfs->super.total_blocks) {
        kprintf("free_block: Invalid block number %d\n", block);
        return;
    }
    
    uint32_t bitmap_start_block = 2;
    
    uint32_t bit_offset = block - dfs->super.data_blocks_start;
    uint32_t sector_offset = bit_offset / (DISK_SECTOR_SIZE * 8);
    uint32_t bit_in_sector = bit_offset % (DISK_SECTOR_SIZE * 8);
    
    uint8_t bitmap_sector[DISK_SECTOR_SIZE];
    if (!diskfs_read_sector(dfs->drive, dfs->start_block + bitmap_start_block + sector_offset, bitmap_sector)) {
        kprintf("free_block: Failed to read bitmap sector %d\n", bitmap_start_block + sector_offset);
        return;
    }
    
    if (!test_bitmap_bit(bitmap_sector, bit_in_sector)) {
        kprintf("free_block: Block %d is already free\n", block);
        return;
    }
    
    clear_bitmap_bit(bitmap_sector, bit_in_sector);
    
    if (!diskfs_write_sector(dfs->drive, dfs->start_block + bitmap_start_block + sector_offset, bitmap_sector)) {
        kprintf("free_block: Failed to write bitmap sector %d\n", bitmap_start_block + sector_offset);
        return;
    }
    
    dfs->super.free_blocks++;
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
        kprintf("free_block: Failed to update superblock\n");
        return;
    }
    
    kprintf("DEBUG: Freed block %d (bit %d in sector %d)\n", 
            block, bit_in_sector, bitmap_start_block + sector_offset);
}

static InodeCacheEntry* get_inode(DiskfsInfo* dfs, uint32_t inode_num) {
    if (inode_num >= dfs->super.total_inodes) {
        kprintf("get_inode: Invalid inode number %d\n", inode_num);
        return NULL;
    }
    
    for (int i = 0; i < INODE_CACHE_SIZE; i++) {
        if (dfs->inode_cache[i].valid && dfs->inode_cache[i].inode_num == inode_num) {
            dfs->inode_cache[i].ref_count++;
            return &dfs->inode_cache[i];
        }
    }
    
    int free_index = -1;
    for (int i = 0; i < INODE_CACHE_SIZE; i++) {
        if (!dfs->inode_cache[i].valid) {
            free_index = i;
            break;
        }
    }
    
    if (free_index < 0) {
        for (int i = 0; i < INODE_CACHE_SIZE; i++) {
            if (dfs->inode_cache[i].ref_count == 0) {
                if (dfs->inode_cache[i].dirty) {
                    flush_inode(dfs, &dfs->inode_cache[i]);
                }
                free_index = i;
                break;
            }
        }
    }
    
    if (free_index < 0) {
        kprintf("get_inode: Cache full and all entries in use\n");
        return NULL;
    }
    
    uint32_t inodes_per_block = DISK_SECTOR_SIZE / sizeof(DiskfsInode);
    uint32_t block_num = dfs->super.inode_table_start + (inode_num / inodes_per_block);
    
    BlockCacheEntry* bce = get_block(dfs, block_num);
    if (!bce) {
        kprintf("get_inode: Failed to read block %d containing inode %d\n", block_num, inode_num);
        return NULL;
    }
    
    uint32_t offset_in_block = (inode_num % inodes_per_block) * sizeof(DiskfsInode);
    memcpy(&dfs->inode_cache[free_index].inode, bce->data + offset_in_block, sizeof(DiskfsInode));
    
    release_block(bce);
    
    dfs->inode_cache[free_index].inode_num = inode_num;
    dfs->inode_cache[free_index].block_num = block_num;
    dfs->inode_cache[free_index].ref_count = 1;
    dfs->inode_cache[free_index].dirty = 0;
    dfs->inode_cache[free_index].valid = 1;
    
    return &dfs->inode_cache[free_index];
}

BlockCacheEntry* get_block(DiskfsInfo* dfs, uint32_t block_num) {
    if (block_num >= dfs->super.total_blocks) {
        kprintf("get_block: Invalid block number %d\n", block_num);
        return NULL;
    }
    
    for (int i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (dfs->block_cache[i].valid && dfs->block_cache[i].block_num == block_num) {
            dfs->block_cache[i].ref_count++;
            return &dfs->block_cache[i];
        }
    }
    
    int free_index = -1;
    for (int i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (!dfs->block_cache[i].valid) {
            free_index = i;
            break;
        }
    }
    
    if (free_index < 0) {
        for (int i = 0; i < BLOCK_CACHE_SIZE; i++) {
            if (dfs->block_cache[i].ref_count == 0) {
                if (dfs->block_cache[i].dirty) {
                    flush_block(dfs, &dfs->block_cache[i]);
                }
                free_index = i;
                break;
            }
        }
    }
    
    if (free_index < 0) {
        kprintf("get_block: Cache full and all entries in use\n");
        return NULL;
    }
    
    if (!diskfs_read_sector(dfs->drive, block_num, dfs->block_cache[free_index].data)) {
        kprintf("get_block: Failed to read block %d\n", block_num);
        return NULL;
    }
    
    dfs->block_cache[free_index].block_num = block_num;
    dfs->block_cache[free_index].ref_count = 1;
    dfs->block_cache[free_index].dirty = 0;
    dfs->block_cache[free_index].valid = 1;
    
    return &dfs->block_cache[free_index];
}

static int flush_inode(DiskfsInfo* dfs, InodeCacheEntry* ice) {
    if (!ice->dirty) return 1;
    
    BlockCacheEntry* bce = get_block(dfs, ice->block_num);
    if (!bce) {
        kprintf("flush_inode: Failed to read block %d\n", ice->block_num);
        return 0;
    }
    
    uint32_t inodes_per_block = DISK_SECTOR_SIZE / sizeof(DiskfsInode);
    uint32_t offset_in_block = (ice->inode_num % inodes_per_block) * sizeof(DiskfsInode);
    
    memcpy(bce->data + offset_in_block, &ice->inode, sizeof(DiskfsInode));
    bce->dirty = 1;
    
    int result = flush_block(dfs, bce);
    
    release_block(bce);
    
    if (result) {
        ice->dirty = 0;
    }
    
    return result;
}

static int flush_block(DiskfsInfo* dfs, BlockCacheEntry* bce) {
    if (!bce->dirty) return 1;
    
    if (!diskfs_write_sector(dfs->drive, bce->block_num, bce->data)) {
        kprintf("flush_block: Failed to write block %d\n", bce->block_num);
        return 0;
    }
    
    bce->dirty = 0;
    return 1;
}

static void release_inode(InodeCacheEntry* ice) {
    if (ice && ice->ref_count > 0) {
        ice->ref_count--;
    }
}

void release_block(BlockCacheEntry* bce) {
    if (bce && bce->ref_count > 0) {
        bce->ref_count--;
    }
}

int init_filesystem(DiskfsInfo* dfs) {
    if (!dfs) return 0;
    
    kprintf("Initializing filesystem on drive %d, start block %d\n", 
            dfs->drive, dfs->start_block);

    uint32_t total_blocks = 8192;
    uint32_t inode_table_start = 3;
    uint32_t inode_table_blocks = 16;
    uint32_t data_blocks_start = inode_table_start + inode_table_blocks;
    uint32_t inodes_per_block = DISK_SECTOR_SIZE / sizeof(DiskfsInode);
    uint32_t total_inodes = inodes_per_block * inode_table_blocks;
    
    dfs->super.magic = DISKFS_MAGIC;
    dfs->super.version = 1;
    dfs->super.block_size = DISK_SECTOR_SIZE;
    dfs->super.total_blocks = total_blocks;
    dfs->super.total_inodes = total_inodes;
    dfs->super.free_blocks = total_blocks - data_blocks_start;
    dfs->super.inode_count = 1;
    dfs->super.inode_table_start = inode_table_start;
    dfs->super.data_blocks_start = data_blocks_start;
    dfs->super.root_inode = 0;
    dfs->super.journal_start = 0;
    dfs->super.journal_blocks = 0;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    
    if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
        kprintf("init_filesystem: Failed to write superblock\n");
        return 0;
    }
    
    uint8_t bitmap_buf[DISK_SECTOR_SIZE];
    memset(bitmap_buf, 0, DISK_SECTOR_SIZE);
    bitmap_buf[0] = 0x01;
    
    if (!diskfs_write_sector(dfs->drive, dfs->start_block + 1, bitmap_buf)) {
        kprintf("init_filesystem: Failed to write inode bitmap\n");
        return 0;
    }
    
    memset(bitmap_buf, 0, DISK_SECTOR_SIZE);
    
    for (uint32_t i = 0; i < data_blocks_start; i++) {
        bitmap_buf[i / 8] |= (1 << (i % 8));
    }
    
    bitmap_buf[data_blocks_start / 8] |= (1 << (data_blocks_start % 8));
    
    if (!diskfs_write_sector(dfs->drive, dfs->start_block + 2, bitmap_buf)) {
        kprintf("init_filesystem: Failed to write block bitmap\n");
        return 0;
    }
    
    memset(bitmap_buf, 0, DISK_SECTOR_SIZE);
    
    for (uint32_t i = 0; i < inode_table_blocks; i++) {
        if (!diskfs_write_sector(dfs->drive, dfs->start_block + inode_table_start + i, bitmap_buf)) {
            kprintf("init_filesystem: Failed to clear inode table block %d\n", i);
            return 0;
        }
    }
    
    DiskfsInode root_inode;
    memset(&root_inode, 0, sizeof(DiskfsInode));
    
    root_inode.mode = 0755 | 0040000;  // drwxr-xr-x
    root_inode.uid = 0;
    root_inode.gid = 0;
    root_inode.size = sizeof(DiskfsDirectory);
    root_inode.links = 1;
    root_inode.flags = INODE_DIRECTORY;
    root_inode.direct[0] = data_blocks_start;
    
    memset(bitmap_buf, 0, DISK_SECTOR_SIZE);
    memcpy(bitmap_buf, &root_inode, sizeof(DiskfsInode));
    
    if (!diskfs_write_sector(dfs->drive, dfs->start_block + inode_table_start, bitmap_buf)) {
        kprintf("init_filesystem: Failed to write root inode\n");
        return 0;
    }
    
    if (!init_directory(dfs, data_blocks_start, 0)) {
        kprintf("init_filesystem: Failed to initialize root directory\n");
        return 0;
    }
    
    if (!add_dir_entry(dfs, 0, ".", 0, INODE_DIRECTORY)) {
        kprintf("init_filesystem: Failed to add '.' entry\n");
        return 0;
    }
    
    if (!add_dir_entry(dfs, 0, "..", 0, INODE_DIRECTORY)) {
        kprintf("init_filesystem: Failed to add '..' entry\n");
        return 0;
    }
    
    kprintf("Filesystem initialized successfully\n");
    return 1;
}

static int init_directory(DiskfsInfo* dfs, uint32_t dir_block, uint32_t parent_inode) {
    if (!dfs || dir_block == 0) {
        kprintf("init_directory: Invalid parameters\n");
        return 0;
    }
    
    uint8_t dir_buf[DISK_SECTOR_SIZE];
    memset(dir_buf, 0, DISK_SECTOR_SIZE);
    
    DiskfsDirectory* dir = (DiskfsDirectory*)dir_buf;
    dir->entry_count = 0;
    dir->parent_inode = parent_inode;
    dir->next_block = 0;
    
    if (!diskfs_write_sector(dfs->drive, dir_block, dir_buf)) {
        kprintf("init_directory: Failed to write directory block %d\n", dir_block);
        return 0;
    }
    
    sleep(5);
    return 1;
}

static int add_dir_entry(DiskfsInfo* dfs, uint32_t dir_inode, const char* name, uint32_t inode_num, uint8_t type) {
    if (!dfs || !name || strlen(name) >= DISKFS_MAX_NAME) {
        kprintf("add_dir_entry: Invalid parameters\n");
        return 0;
    }
    
    InodeCacheEntry* dir_ice = get_inode(dfs, dir_inode);
    if (!dir_ice) {
        kprintf("add_dir_entry: Failed to get directory inode %d\n", dir_inode);
        return 0;
    }
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("add_dir_entry: Inode %d is not a directory\n", dir_inode);
        release_inode(dir_ice);
        return 0;
    }
    
    uint32_t block_num = 0;
    
    for (uint32_t idx = 0; idx < DIRECT_BLOCKS; idx++) {
        if (dir_ice->inode.direct[idx] != 0) {
            BlockCacheEntry* bce = get_block(dfs, dir_ice->inode.direct[idx]);
            if (!bce) continue;
            
            DiskfsDirectory* dir = (DiskfsDirectory*)bce->data;
            uint32_t max_entries = (DISK_SECTOR_SIZE - sizeof(DiskfsDirectory)) / sizeof(DiskfsDirEntry);
            
            if (dir->entry_count < max_entries) {
                block_num = dir_ice->inode.direct[idx];
                release_block(bce);
                break;
            }
            
            if (dir->next_block != 0) {
                uint32_t next = dir->next_block;
                release_block(bce);
                
                while (next != 0) {
                    BlockCacheEntry* next_bce = get_block(dfs, next);
                    if (!next_bce) break;
                    
                    DiskfsDirectory* next_dir = (DiskfsDirectory*)next_bce->data;
                    if (next_dir->entry_count < max_entries) {
                        block_num = next;
                        release_block(next_bce);
                        break;
                    }
                    
                    uint32_t tmp = next_dir->next_block;
                    release_block(next_bce);
                    next = tmp;
                }
                
                if (block_num != 0) break;
            } else {
                release_block(bce);
            }
        }
    }
    
    if (block_num == 0) {
        uint32_t empty_idx = DIRECT_BLOCKS;
        for (uint32_t idx = 0; idx < DIRECT_BLOCKS; idx++) {
            if (dir_ice->inode.direct[idx] == 0) {
                empty_idx = idx;
                break;
            }
        }
        
        if (empty_idx == DIRECT_BLOCKS) {
            uint32_t last_block = 0;
            for (uint32_t idx = 0; idx < DIRECT_BLOCKS; idx++) {
                if (dir_ice->inode.direct[idx] != 0) {
                    last_block = dir_ice->inode.direct[idx];
                    
                    BlockCacheEntry* bce = get_block(dfs, last_block);
                    if (!bce) continue;
                    
                    DiskfsDirectory* dir = (DiskfsDirectory*)bce->data;
                    while (dir->next_block != 0) {
                        uint32_t next = dir->next_block;
                        release_block(bce);
                        last_block = next;
                        bce = get_block(dfs, last_block);
                        if (!bce) break;
                        dir = (DiskfsDirectory*)bce->data;
                    }
                    
                    if (bce) release_block(bce);
                    break;
                }
            }
            
            if (last_block == 0) {
                kprintf("add_dir_entry: No blocks in directory!\n");
                release_inode(dir_ice);
                return 0;
            }
            
            uint32_t new_block = allocate_block(dfs);
            if (new_block == 0) {
                kprintf("add_dir_entry: Failed to allocate directory block\n");
                release_inode(dir_ice);
                return 0;
            }
            
            if (!init_directory(dfs, new_block, dir_inode)) {
                kprintf("add_dir_entry: Failed to initialize directory block\n");
                free_block(dfs, new_block);
                release_inode(dir_ice);
                return 0;
            }
            
            BlockCacheEntry* bce = get_block(dfs, last_block);
            if (!bce) {
                kprintf("add_dir_entry: Failed to read last directory block\n");
                free_block(dfs, new_block);
                release_inode(dir_ice);
                return 0;
            }
            
            DiskfsDirectory* dir = (DiskfsDirectory*)bce->data;
            dir->next_block = new_block;
            bce->dirty = 1;
            flush_block(dfs, bce);
            release_block(bce);
            
            block_num = new_block;
        } else {
            uint32_t new_block = allocate_block(dfs);
            if (new_block == 0) {
                kprintf("add_dir_entry: Failed to allocate directory block\n");
                release_inode(dir_ice);
                return 0;
            }
            
            if (!init_directory(dfs, new_block, dir_inode)) {
                kprintf("add_dir_entry: Failed to initialize directory block\n");
                free_block(dfs, new_block);
                release_inode(dir_ice);
                return 0;
            }
            
            dir_ice->inode.direct[empty_idx] = new_block;
            dir_ice->dirty = 1;
            
            block_num = new_block;
        }
    }
    
    BlockCacheEntry* bce = get_block(dfs, block_num);
    if (!bce) {
        kprintf("add_dir_entry: Failed to read directory block %d\n", block_num);
        release_inode(dir_ice);
        return 0;
    }
    
    DiskfsDirectory* dir = (DiskfsDirectory*)bce->data;
    DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));
    
    for (uint32_t i = 0; i < dir->entry_count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            kprintf("add_dir_entry: Entry '%s' already exists\n", name);
            release_block(bce);
            release_inode(dir_ice);
            return 0;
        }
    }
    
    DiskfsDirEntry* new_entry = &entries[dir->entry_count];
    strcpy(new_entry->name, name);
    new_entry->inode = inode_num;
    new_entry->type = type;
    
    dir->entry_count++;
    bce->dirty = 1;
    
    if (dir_ice->inode.size < sizeof(DiskfsDirectory) + dir->entry_count * sizeof(DiskfsDirEntry)) {
        dir_ice->inode.size = sizeof(DiskfsDirectory) + dir->entry_count * sizeof(DiskfsDirEntry);
        dir_ice->dirty = 1;
    }
    
    flush_block(dfs, bce);
    flush_inode(dfs, dir_ice);
    
    release_block(bce);
    release_inode(dir_ice);
    
    return 1;
}

static int remove_dir_entry(DiskfsInfo* dfs, uint32_t dir_inode, const char* name) {
    if (!name) {
        kprintf("remove_dir_entry: Invalid name\n");
        return 0;
    }
    
    InodeCacheEntry* dir_ice = get_inode(dfs, dir_inode);
    if (!dir_ice) {
        kprintf("remove_dir_entry: Failed to get directory inode %d\n", dir_inode);
        return 0;
    }
    
    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
        kprintf("remove_dir_entry: Inode %d is not a directory\n", dir_inode);
        release_inode(dir_ice);
        return 0;
    }
    
    uint32_t block_idx = 0;
    bool entry_removed = false;
    uint32_t prev_block = 0;
    uint32_t current_block = 0;
    
    while (block_idx < DIRECT_BLOCKS && !entry_removed) {
        current_block = dir_ice->inode.direct[block_idx];
        if (current_block == 0) {
            break;
        }
        
        BlockCacheEntry* bce = get_block(dfs, current_block);
        if (!bce) {
            kprintf("remove_dir_entry: Failed to get directory block %d\n", current_block);
            release_inode(dir_ice);
            return 0;
        }
        
        DiskfsDirectory* dir = (DiskfsDirectory*)bce->data;
        DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));
        
        for (uint32_t i = 0; i < dir->entry_count; i++) {
            if (strcmp(entries[i].name, name) == 0) {
                if (i < dir->entry_count - 1) {
                    memmove(&entries[i], &entries[i + 1], 
                            (dir->entry_count - i - 1) * sizeof(DiskfsDirEntry));
                }
                
                dir->entry_count--;
                bce->dirty = 1;
                
                if (dir->entry_count == 0 && prev_block != 0) {
                    BlockCacheEntry* prev_bce = get_block(dfs, prev_block);
                    if (prev_bce) {
                        DiskfsDirectory* prev_dir = (DiskfsDirectory*)prev_bce->data;
                        prev_dir->next_block = dir->next_block;
                        prev_bce->dirty = 1;
                        flush_block(dfs, prev_bce);
                        release_block(prev_bce);
                    }
                    
                    release_block(bce);
                    free_block(dfs, current_block);
                    
                    if (block_idx < DIRECT_BLOCKS && dir_ice->inode.direct[block_idx] == current_block) {
                        dir_ice->inode.direct[block_idx] = 0;
                        dir_ice->dirty = 1;
                    }
                } else {
                    flush_block(dfs, bce);
                    release_block(bce);
                }
                
                entry_removed = true;
                break;
            }
        }
        
        if (!entry_removed) {
            uint32_t next_block = dir->next_block;
            release_block(bce);
            
            if (next_block != 0) {
                prev_block = current_block;
                current_block = next_block;
                continue;
            }
            
            prev_block = 0;
            block_idx++;
        }
    }
    
    if (dir_ice->dirty) {
        flush_inode(dfs, dir_ice);
    }
    
    release_inode(dir_ice);
    
    return entry_removed ? 1 : 0;
}

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t block_count;
    uint32_t checksum;
    uint32_t state;
    uint32_t head;
    uint32_t tail;
} DiskfsJournalHeader;

typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint16_t size;
    uint32_t sequence;
    uint32_t checksum;
} DiskfsJournalTransaction;

static int init_journal(DiskfsInfo* dfs, uint32_t journal_size) {
    if (!dfs || journal_size < 4) {
        kprintf("init_journal: Invalid parameters\n");
        return 0;
    }
    
    uint32_t journal_start = 0;
    uint32_t journal_blocks[journal_size];
    
    for (uint32_t i = 0; i < journal_size; i++) {
        journal_blocks[i] = allocate_block(dfs);
        if (journal_blocks[i] == 0) {
            for (uint32_t j = 0; j < i; j++) {
                free_block(dfs, journal_blocks[j]);
            }
            kprintf("init_journal: Failed to allocate journal blocks\n");
            return 0;
        }
        
        if (i == 0) {
            journal_start = journal_blocks[i];
        }
    }
    
    BlockCacheEntry* header_bce = get_block(dfs, journal_start);
    if (!header_bce) {
        for (uint32_t i = 0; i < journal_size; i++) {
            free_block(dfs, journal_blocks[i]);
        }
        kprintf("init_journal: Failed to get journal header block\n");
        return 0;
    }
    
    DiskfsJournalHeader* jh = (DiskfsJournalHeader*)header_bce->data;
    jh->magic = JOURNAL_MAGIC;
    jh->sequence = 1;
    jh->block_count = journal_size;
    jh->checksum = 0;
    jh->state = 0;
    jh->head = journal_start;
    jh->tail = journal_start;
    
    header_bce->dirty = 1;
    flush_block(dfs, header_bce);
    release_block(header_bce);
    
    dfs->super.journal_start = journal_start;
    dfs->super.journal_blocks = journal_size;
    
    uint8_t sb_buf[DISK_SECTOR_SIZE];
    memset(sb_buf, 0, DISK_SECTOR_SIZE);
    memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
    if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
        kprintf("init_journal: Failed to update superblock\n");
        return 0;
    }
    
    kprintf("init_journal: Journal initialized with %d blocks starting at %d\n", 
            journal_size, journal_start);
    return 1;
}

static int journal_start_transaction(DiskfsInfo* dfs) {
    if (!dfs || dfs->super.journal_start == 0) {
        kprintf("journal_start_transaction: No journal configured\n");
        return 0;
    }
    
    BlockCacheEntry* header_bce = get_block(dfs, dfs->super.journal_start);
    if (!header_bce) {
        kprintf("journal_start_transaction: Failed to read journal header\n");
        return 0;
    }
    
    DiskfsJournalHeader* jh = (DiskfsJournalHeader*)header_bce->data;
    
    if (jh->state != 0) {
        kprintf("journal_start_transaction: Journal not in clean state, recovery needed\n");
        release_block(header_bce);
        return 0;
    }
    
    jh->state = 1;
    jh->sequence++;
    header_bce->dirty = 1;
    flush_block(dfs, header_bce);
    
    uint32_t next_block = (jh->head == dfs->super.journal_start) ? 
                          dfs->super.journal_start + 1 : 
                          jh->head + 1;
    
    if (next_block >= dfs->super.journal_start + dfs->super.journal_blocks) {
        next_block = dfs->super.journal_start + 1;
    }
    
    BlockCacheEntry* tx_bce = get_block(dfs, next_block);
    if (!tx_bce) {
        kprintf("journal_start_transaction: Failed to get transaction block\n");
        release_block(header_bce);
        return 0;
    }
    
    DiskfsJournalTransaction* tx = (DiskfsJournalTransaction*)tx_bce->data;
    tx->type = JT_START;
    tx->flags = 0;
    tx->size = sizeof(DiskfsJournalTransaction);
    tx->sequence = jh->sequence;
    tx->checksum = 0;
    
    tx_bce->dirty = 1;
    flush_block(dfs, tx_bce);
    release_block(tx_bce);
    
    jh->head = next_block;
    header_bce->dirty = 1;
    flush_block(dfs, header_bce);
    release_block(header_bce);
    
    return 1;
}

static int journal_commit_transaction(DiskfsInfo* dfs) {
    if (!dfs || dfs->super.journal_start == 0) {
        kprintf("journal_commit_transaction: No journal configured\n");
        return 0;
    }
    
    BlockCacheEntry* header_bce = get_block(dfs, dfs->super.journal_start);
    if (!header_bce) {
        kprintf("journal_commit_transaction: Failed to read journal header\n");
        return 0;
    }
    
    DiskfsJournalHeader* jh = (DiskfsJournalHeader*)header_bce->data;
    
    if (jh->state != 1) {
        kprintf("journal_commit_transaction: No active transaction\n");
        release_block(header_bce);
        return 0;
    }
    
    uint32_t next_block = (jh->head == dfs->super.journal_start) ? 
                          dfs->super.journal_start + 1 : 
                          jh->head + 1;
    
    if (next_block >= dfs->super.journal_start + dfs->super.journal_blocks) {
        next_block = dfs->super.journal_start + 1;
    }
    
    BlockCacheEntry* tx_bce = get_block(dfs, next_block);
    if (!tx_bce) {
        kprintf("journal_commit_transaction: Failed to get transaction block\n");
        release_block(header_bce);
        return 0;
    }
    
    DiskfsJournalTransaction* tx = (DiskfsJournalTransaction*)tx_bce->data;
    tx->type = JT_COMMIT;
    tx->flags = 0;
    tx->size = sizeof(DiskfsJournalTransaction);
    tx->sequence = jh->sequence;
    tx->checksum = 0;
    
    tx_bce->dirty = 1;
    flush_block(dfs, tx_bce);
    release_block(tx_bce);
    
    jh->head = next_block;
    jh->state = 0;
    header_bce->dirty = 1;
    flush_block(dfs, header_bce);
    release_block(header_bce);
    
    return 1;
}

static int journal_log_block(DiskfsInfo* dfs, uint32_t block_num) {
    if (!dfs || dfs->super.journal_start == 0 || block_num == 0) {
        return 0;
    }
    
    BlockCacheEntry* header_bce = get_block(dfs, dfs->super.journal_start);
    if (!header_bce) {
        kprintf("journal_log_block: Failed to read journal header\n");
        return 0;
    }
    
    DiskfsJournalHeader* jh = (DiskfsJournalHeader*)header_bce->data;
    
    if (jh->state != 1) {
        kprintf("journal_log_block: No active transaction\n");
        release_block(header_bce);
        return 0;
    }
    
    BlockCacheEntry* src_bce = get_block(dfs, block_num);
    if (!src_bce) {
        kprintf("journal_log_block: Failed to read block %d\n", block_num);
        release_block(header_bce);
        return 0;
    }
    
    uint32_t next_block = (jh->head == dfs->super.journal_start) ? 
                         dfs->super.journal_start + 1 : 
                         jh->head + 1;
    
    if (next_block >= dfs->super.journal_start + dfs->super.journal_blocks) {
        next_block = dfs->super.journal_start + 1;
    }
    
    BlockCacheEntry* tx_bce = get_block(dfs, next_block);
    if (!tx_bce) {
        kprintf("journal_log_block: Failed to get transaction block\n");
        release_block(header_bce);
        release_block(src_bce);
        return 0;
    }
    
    DiskfsJournalTransaction* tx = (DiskfsJournalTransaction*)tx_bce->data;
    tx->type = JT_BLOCK;
    tx->flags = 0;
    tx->size = sizeof(DiskfsJournalTransaction) + sizeof(uint32_t) + DISK_SECTOR_SIZE;
    tx->sequence = jh->sequence;
    
    uint32_t* block_ptr = (uint32_t*)(tx_bce->data + sizeof(DiskfsJournalTransaction));
    *block_ptr = block_num;
    
    memcpy(tx_bce->data + sizeof(DiskfsJournalTransaction) + sizeof(uint32_t), 
           src_bce->data, DISK_SECTOR_SIZE);
    
    tx_bce->dirty = 1;
    flush_block(dfs, tx_bce);
    release_block(tx_bce);
    release_block(src_bce);
    
    jh->head = next_block;
    header_bce->dirty = 1;
    flush_block(dfs, header_bce);
    release_block(header_bce);
    
    return 1;
}

static uint32_t allocate_extent(DiskfsInfo* dfs, uint32_t count, uint32_t* allocated) {
    if (!dfs || count == 0 || !allocated) {
        *allocated = 0;
        return 0;
    }
    
    uint32_t bitmap_blocks = dfs->super.bitmap_blocks;
    uint32_t bitmap_start = dfs->super.bitmap_start_block;
    uint32_t total_blocks = dfs->super.total_blocks;
    
    if (dfs->super.free_blocks < count) {
        kprintf("allocate_extent: Not enough free blocks available (need %d, have %d)\n",
                count, dfs->super.free_blocks);
        *allocated = 0;
        return 0;
    }
    
    uint8_t bitmap_sector[DISK_SECTOR_SIZE];
    uint32_t start_block = 0;
    uint32_t current_run = 0;
    *allocated = 0;
    
    for (uint32_t bitmap_block = 0; bitmap_block < bitmap_blocks; bitmap_block++) {
        uint32_t sector = bitmap_start + bitmap_block;
        
        if (!diskfs_read_sector(dfs->drive, sector, bitmap_sector)) {
            kprintf("allocate_extent: Failed to read bitmap sector %d\n", sector);
            *allocated = 0;
            return 0;
        }
        
        for (uint32_t i = 0; i < DISK_SECTOR_SIZE * 8; i++) {
            uint32_t block = bitmap_block * BLOCKS_PER_BITMAP_SECTOR + i;
            
            if (block >= total_blocks) break;
            
            if (block < dfs->super.data_blocks_start) continue;
            
            if (!test_bitmap_bit(bitmap_sector, i)) {
                if (current_run == 0) {
                    start_block = block;
                }
                current_run++;
                
                if (current_run == count) {
                    *allocated = count;
                    break;
                }
            } else {
                current_run = 0;
                start_block = 0;
            }
        }
        
        if (*allocated > 0) break;
    }
    
    if (current_run > 0 && *allocated == 0) {
        *allocated = current_run;
    }
    
    if (*allocated > 0) {
        for (uint32_t i = 0; i < *allocated; i++) {
            uint32_t block = start_block + i;
            uint32_t bitmap_block = block / BLOCKS_PER_BITMAP_SECTOR;
            uint32_t bit_in_block = block % BLOCKS_PER_BITMAP_SECTOR;
            uint32_t sector = bitmap_start + bitmap_block;
            
            if (!diskfs_read_sector(dfs->drive, sector, bitmap_sector)) {
                kprintf("allocate_extent: Failed to read bitmap sector %d\n", sector);
                // This is bad - already allocated some blocks
                // Should initiate recovery here
                return 0;
            }
            
            set_bitmap_bit(bitmap_sector, bit_in_block);
            
            if (!diskfs_write_sector(dfs->drive, sector, bitmap_sector)) {
                kprintf("allocate_extent: Failed to write bitmap sector %d\n", sector);
                return 0;
            }
            
            uint8_t zero_buf[DISK_SECTOR_SIZE];
            memset(zero_buf, 0, DISK_SECTOR_SIZE);
            if (!diskfs_write_sector(dfs->drive, block, zero_buf)) {
                kprintf("allocate_extent: Failed to initialize block %d\n", block);
                return 0;
            }
        }
        
        dfs->super.free_blocks -= *allocated;
        uint8_t sb_buf[DISK_SECTOR_SIZE];
        memset(sb_buf, 0, DISK_SECTOR_SIZE);
        memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
        if (!diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf)) {
            kprintf("allocate_extent: Failed to update superblock\n");
            return 0;
        }
        
        kprintf("DEBUG: Allocated extent of %d blocks starting at %d\n", 
                *allocated, start_block);
    }
    
    return start_block;
}

static void free_extent(DiskfsInfo* dfs, uint32_t start_block, uint32_t count) {
    if (!dfs || start_block == 0 || count == 0) {
        return;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        free_block(dfs, start_block + i);
    }
}

static void flush_all_cache(DiskfsInfo* dfs) {
    if (!dfs) return;
    
    for (int i = 0; i < INODE_CACHE_SIZE; i++) {
        if (dfs->inode_cache[i].valid && dfs->inode_cache[i].dirty) {
            flush_inode(dfs, &dfs->inode_cache[i]);
        }
    }
    
    for (int i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (dfs->block_cache[i].valid && dfs->block_cache[i].dirty) {
            flush_block(dfs, &dfs->block_cache[i]);
        }
    }
}

static void prefetch_blocks(DiskfsInfo* dfs, uint32_t start_block, uint32_t count) {
    if (!dfs || start_block == 0 || count == 0) {
        return;
    }
    
    if (count > 8) count = 8;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t block = start_block + i;
        
        bool in_cache = false;
        for (int j = 0; j < BLOCK_CACHE_SIZE; j++) {
            if (dfs->block_cache[j].valid && 
                dfs->block_cache[j].block_num == block) {
                in_cache = true;
                break;
            }
        }
        
        if (!in_cache) {
            int free_index = -1;
            for (int j = 0; j < BLOCK_CACHE_SIZE; j++) {
                if (!dfs->block_cache[j].valid) {
                    free_index = j;
                    break;
                }
            }
            
            if (free_index >= 0) {
                if (diskfs_read_sector(dfs->drive, block, 
                                       dfs->block_cache[free_index].data)) {
                    dfs->block_cache[free_index].block_num = block;
                    dfs->block_cache[free_index].ref_count = 0;
                    dfs->block_cache[free_index].dirty = 0;
                    dfs->block_cache[free_index].valid = 1;
                }
            }
        }
    }
}

static uint32_t calculate_checksum(const void* data, uint32_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (uint32_t i = 0; i < size; i++) {
        checksum = ((checksum << 5) | (checksum >> 27)) ^ bytes[i];
    }
    
    return checksum;
}

static int verify_superblock(DiskfsInfo* dfs) {
    if (!dfs) return 0;
    
    if (dfs->super.magic != DISKFS_MAGIC) {
        kprintf("verify_superblock: Invalid magic number\n");
        return 0;
    }
    
    if (dfs->super.total_blocks < dfs->super.data_blocks_start) {
        kprintf("verify_superblock: Inconsistent block counts\n");
        return 0;
    }
    
    return 1;
}

static int verify_bitmap(DiskfsInfo* dfs) {
    if (!dfs) return 0;
    
    uint32_t bitmap_blocks = dfs->super.bitmap_blocks;
    uint32_t bitmap_start = dfs->super.bitmap_start_block;
    uint32_t total_blocks = dfs->super.total_blocks;
    uint32_t free_blocks_count = 0;
    
    uint8_t bitmap_sector[DISK_SECTOR_SIZE];
    
    for (uint32_t bitmap_block = 0; bitmap_block < bitmap_blocks; bitmap_block++) {
        uint32_t sector = bitmap_start + bitmap_block;
        
        if (!diskfs_read_sector(dfs->drive, sector, bitmap_sector)) {
            kprintf("verify_bitmap: Failed to read bitmap sector %d\n", sector);
            return 0;
        }
        
        for (uint32_t i = 0; i < DISK_SECTOR_SIZE * 8; i++) {
            uint32_t block = bitmap_block * BLOCKS_PER_BITMAP_SECTOR + i;
            
            if (block >= total_blocks) break;
            
            if (!test_bitmap_bit(bitmap_sector, i)) {
                free_blocks_count++;
            }
        }
    }
    
    if (free_blocks_count != dfs->super.free_blocks) {
        kprintf("verify_bitmap: Bitmap inconsistency - counted %d free blocks but superblock says %d\n",
                free_blocks_count, dfs->super.free_blocks);
        dfs->super.free_blocks = free_blocks_count;
        uint8_t sb_buf[DISK_SECTOR_SIZE];
        memset(sb_buf, 0, DISK_SECTOR_SIZE);
        memcpy(sb_buf, &dfs->super, sizeof(DiskfsSuper));
        diskfs_write_sector(dfs->drive, dfs->start_block, sb_buf);
        return 0;
    }
    
    return 1;
}

static int check_filesystem(DiskfsInfo* dfs, int fix_errors) {
    if (!dfs) return 0;
    
    int result = 1;
    
    if (!verify_superblock(dfs)) {
        kprintf("check_filesystem: Superblock verification failed\n");
        result = 0;
    }
    
    if (!verify_bitmap(dfs)) {
        kprintf("check_filesystem: Bitmap verification failed\n");
        result = 0;
    }
    
    // TODO: Check inode consistency, directory structure, etc.
    
    if (!result && fix_errors) {
        // Implement repair logic here
        kprintf("check_filesystem: Attempting to repair filesystem errors\n");
    }
    
    return result;
}

static int diskfs_truncate(Inode* inode, uint64_t size) {
    if (!inode) return 0;
    
    DiskfsInfo* dfs = (DiskfsInfo*)inode->sb->fs_specific;
    InodeCacheEntry* ice = (InodeCacheEntry*)inode->fs_specific;
    
    if (size == inode->size) {
        return 1;
    }
    
    if (size != 0) {
        return 0;
    }
    
    for (int i = 0; i < DIRECT_BLOCKS; i++) {
        if (ice->inode.direct[i] != 0) {
            free_block(dfs, ice->inode.direct[i]);
            ice->inode.direct[i] = 0;
        }
    }
    
    if (ice->inode.indirect != 0) {
        BlockCacheEntry* indirect_bce = get_block(dfs, ice->inode.indirect);
        if (indirect_bce) {
            uint32_t* block_ptrs = (uint32_t*)indirect_bce->data;
            uint32_t blocks_per_indirect = dfs->super.block_size / sizeof(uint32_t);
            
            for (uint32_t i = 0; i < blocks_per_indirect; i++) {
                if (block_ptrs[i] != 0) {
                    free_block(dfs, block_ptrs[i]);
                }
            }
            
            release_block(indirect_bce);
            free_block(dfs, ice->inode.indirect);
            ice->inode.indirect = 0;
        }
    }
    
    ice->inode.size = 0;
    inode->size = 0;
    
    ice->dirty = 1;
    flush_inode(dfs, ice);
    
    return 1;
}