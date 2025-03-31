#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "../../kernel/drivers/vga/vga.h"
#include "commands.h"

void ls(char* args) {
    Inode* current_dir = get_current_dir();
    if (!current_dir) {
        kprint("ls: Cannot access current directory\n");
    } else {
        DiskfsInfo* dfs = (DiskfsInfo*)current_dir->sb->fs_specific;
        InodeCacheEntry* dir_ice = (InodeCacheEntry*)current_dir->fs_specific;

        if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
            kprint("ls: Current path is not a directory\n");
        } else {
            kprintf("Directory contents:\n");

            for (uint32_t block_idx = 0; block_idx < DIRECT_BLOCKS; block_idx++) {
            uint32_t block_num = dir_ice->inode.direct[block_idx];
            if (block_num == 0) return;

            BlockCacheEntry* bce = get_block(dfs, block_num);
            if (!bce) return;

            DiskfsDirectory* dir_header = (DiskfsDirectory*)bce->data;
            DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));

            for (uint32_t i = 0; i < dir_header->entry_count; i++) {
                bool is_dir = (entries[i].type == INODE_DIRECTORY);

                set_color(is_dir ? LIGHT_CYAN : LIGHT_GREEN);

                kprintf("  %s%s\n", entries[i].name, is_dir ? "/" : "");
            }

            uint32_t next_block = dir_header->next_block;
            release_block(bce);

            while (next_block != 0) {
                BlockCacheEntry* next_bce = get_block(dfs, next_block);
                if (!next_bce) break;

                DiskfsDirectory* next_dir = (DiskfsDirectory*)next_bce->data;
                DiskfsDirEntry* next_entries = (DiskfsDirEntry*)(next_bce->data + sizeof(DiskfsDirectory));

                for (uint32_t i = 0; i < next_dir->entry_count; i++) {
                    bool is_dir = (next_entries[i].type == INODE_DIRECTORY);
                    set_color(is_dir ? LIGHT_CYAN : LIGHT_GREEN);
                    kprintf("  %s%s\n", next_entries[i].name, is_dir ? "/" : "");
                }

                uint32_t tmp_next = next_dir->next_block;
                release_block(next_bce);
                next_block = tmp_next;
                }
            }

        set_color(LIGHT_GREEN);
        }
    }
}