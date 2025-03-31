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
        
        kfree(check);
        
        int result = current_dir->ops->delete(current_dir, args);
        if (result) {
            kprintf("'%s' deleted successfully\n", args);
        } else {
            kprintf("Failed to delete '%s'\n", args);
        }
    }
}