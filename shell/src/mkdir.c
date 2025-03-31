#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "commands.h"

void mkdir(char* args) {
    if (args[0] == '\0') {
        kprint("Usage: mkdir <directory-name>\n");
    } else {
        bool valid_name = true;
        for (int i = 0; args[i]; i++) {
            if (args[i] == '/' || args[i] == '\\' || args[i] == ':' ||
                args[i] == '*' || args[i] == '?' || args[i] == '"' ||
                args[i] == '<' || args[i] == '>' || args[i] == '|') {
                valid_name = false;
                break;
            }
        }

        if (!valid_name) {
            kprintf("Invalid directory name: '%s'\n", args);
            kprintf("Names cannot contain: / \\ : * ? \" < > |\n");
            return;
        }

        if (strlen(args) >= DISKFS_MAX_NAME) {
            kprintf("Directory name too long (max %d characters)\n", DISKFS_MAX_NAME - 1);
            return;
        }

        Inode* current_dir = get_current_dir();
        if (!current_dir) {
            kprintf("Error: Cannot access current directory\n");
            return;
        }

        if (!current_dir->ops || !current_dir->ops->mkdir) {
            kprintf("Error: mkdir operation not available\n");
            return;
        }

        Inode* check = NULL;
        if (current_dir->ops->lookup(current_dir, args, &check)) {
            kprintf("Error: '%s' already exists\n", args);
            kfree(check);
            return;
        }

        int result = current_dir->ops->mkdir(current_dir, args, 0755);
        if (result) {
            //kprintf("Directory '%s' created successfully\n", args);
        } else {
            kprintf("Failed to create directory '%s'\n", args);
            DiskfsInfo* dfs = (DiskfsInfo*)current_dir->sb->fs_specific;
            InodeCacheEntry* dir_ice = (InodeCacheEntry*)current_dir->fs_specific;
            kprintf("Debug: inode_count=%d, free_inodes=%d\n",
                   dfs->super.inode_count,
                   dfs->super.total_inodes - dfs->super.inode_count);
        }
    }
}