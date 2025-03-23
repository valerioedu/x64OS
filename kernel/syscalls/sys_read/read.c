#include "read.h"
#include "../../drivers/keyboard/keyboard.h"

ssize_t read(int fd, void* buf, size_t nbyte) {
    if (fd == stdin) {
        char* line = keyboard_read_line();
        if (line == NULL) return 0;
        
        size_t len = strlen(line);
        size_t to_copy = len < nbyte ? len : nbyte;
        memcpy(buf, line, to_copy);
        return to_copy;
    }
    return -1;
}