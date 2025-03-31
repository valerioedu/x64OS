#include "read.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../fs/src/file.h"
#include "../io.h"

ssize_t read(int fd, void* buf, size_t nbyte) {
    if (fd == stdin) {
        char* line = keyboard_read_line();
        if (line == NULL) return 0;
        
        size_t len = strlen(line);
        size_t to_copy = len < nbyte ? len : nbyte;
        memcpy(buf, line, to_copy);
        return to_copy;
    }
    else if (fd == stdout || fd == stderr) {
        return -1;
    }
    else if (fd >= 3 < MAX_OPEN_FILES + 3) {
        int fs_fd = fd - 3;
        return file_read(fs_fd, buf, nbyte);
    }
    return -1;
}