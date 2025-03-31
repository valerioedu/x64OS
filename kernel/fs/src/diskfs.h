#ifndef DISKFS_H
#define DISKFS_H

#include "vfs.h"
#include "../../../lib/definitions.h"

#define DISK_SECTOR_SIZE     512
#define DISKFS_MAGIC         0x46534B44
#define DISKFS_MAX_NAME      60

#define INODE_DIRECTORY      0x01
#define INODE_FILE           0x02
#define JOURNAL_MAGIC 0x4A524E4C
#define JT_START      1
#define JT_COMMIT     2
#define JT_BLOCK      3 

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t access_time;
    uint64_t modify_time;
    uint64_t create_time;
    uint32_t links;
    uint32_t flags;
    uint32_t direct[10];
    uint32_t indirect;
    uint32_t double_indirect;
} DiskfsInode;

typedef struct {
    char name[DISKFS_MAX_NAME];
    uint32_t inode;
    uint8_t type;
} DiskfsDirEntry;

typedef struct {
    uint32_t entry_count;
    uint32_t parent_inode;
    uint32_t next_block;
} DiskfsDirectory;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t inode_count;
    uint32_t total_inodes;
    uint32_t root_inode;
    uint32_t bitmap_start_block;
    uint32_t bitmap_blocks;
    uint32_t inode_table_start;
    uint32_t inode_table_blocks;
    uint32_t data_blocks_start;
    uint64_t mount_time;
    uint64_t write_time;
    uint32_t mount_count;
    uint32_t dirty;
    uint32_t journal_start;
    uint32_t journal_blocks;
} DiskfsSuper;

typedef struct {
    uint32_t inode_num;
    uint32_t block_num;
    DiskfsInode inode;
    uint32_t ref_count;
    uint8_t dirty;
    uint8_t valid;
} InodeCacheEntry;

typedef struct {
    uint32_t block_num;
    uint8_t data[DISK_SECTOR_SIZE];
    uint32_t ref_count;
    uint8_t dirty;
    uint8_t valid;
} BlockCacheEntry;

#define INODE_CACHE_SIZE 16
#define BLOCK_CACHE_SIZE 32
#define DIRECT_BLOCKS    10
#define BLOCKS_PER_BITMAP_SECTOR (DISK_SECTOR_SIZE * 8)

extern Inode* root_inode;
extern Inode* current_directory;

typedef struct {
    uint8_t drive;
    uint32_t start_block;
    DiskfsSuper super;
    InodeCacheEntry inode_cache[INODE_CACHE_SIZE];
    BlockCacheEntry block_cache[BLOCK_CACHE_SIZE];
} DiskfsInfo;

SuperBlock* diskfs_mount(uint8_t drive, uint32_t start_block, int auto_format);
int diskfs_read_sector(uint8_t drive, uint32_t lba, void* buffer);
BlockCacheEntry* get_block(DiskfsInfo* dfs, uint32_t block_num);
void release_block(BlockCacheEntry* bce);

static inline DiskfsInfo* get_diskfs_info(SuperBlock* sb) {
    return (DiskfsInfo*)sb->fs_specific;
}

static inline InodeCacheEntry* get_diskfs_inode(Inode* inode) {
    return (InodeCacheEntry*)inode->fs_specific;
}

#endif