#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "commands.h"

void rm(char* args) {
    if (args[0] == '\0') {
        kprint("Usage: rm <filename>\n");
    } else {
        Inode* current_dir = get_current_dir();
        if (!current_dir) {
            kprintf("Error: Cannot access current directory\n");
            return;
        }
        
        Inode* check = NULL;
        if (!current_dir->ops->lookup(current_dir, args, &check)) {
            kprintf("Error: '%s' does not exist\n", args);
            return;
        }
        
        InodeCacheEntry* target_ice = (InodeCacheEntry*)check->fs_specific;
        if (target_ice && (target_ice->inode.flags & INODE_DIRECTORY)) {
            kprintf("Error: '%s' is a directory, use rmdir instead\n", args);
            kfree(check);
            return;
        }

        kfree(check);
        
        int result = current_dir->ops->delete(current_dir, args);
        if (result) {
            //kprintf("'%s' deleted successfully\n", args);
        } else {
            kprintf("Failed to delete '%s'\n", args);
        }
    }
}

void rmdir(char* args) {
    if (args[0] == '\0') {
        kprint("Usage: rmdir <directory>\n");
    } else {
        Inode* current_dir = get_current_dir();
        if (!current_dir) {
            kprintf("Error: Cannot access current directory\n");
            return;
        }
        
        Inode* check = NULL;
        if (!current_dir->ops->lookup(current_dir, args, &check)) {
            kprintf("Error: '%s' does not exist\n", args);
            return;
        }
        
        InodeCacheEntry* target_ice = (InodeCacheEntry*)check->fs_specific;
        if (target_ice && (target_ice->inode.flags & INODE_FILE)) {
            kprintf("Error: '%s' is a file, use rm instead\n", args);
            kfree(check);
            return;
        }

        kfree(check);
        
        int result = current_dir->ops->delete(current_dir, args);
        if (result) {
            //kprintf("'%s' deleted successfully\n", args);
        } else {
            kprintf("Failed to delete '%s'\n", args);
        }
    }
}