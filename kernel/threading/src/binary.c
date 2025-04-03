#include "../../../lib/definitions.h"
#include "binary.h"
#include "../../mm/heap.h"
#include "../../fs/fs.h"
#include "../../syscalls/sys.h"

int load_binary(const char* path, uint64_t* entry_point) {
    int fd = open(path, 0, 0644);
    if (fd < 0) return -1;
    
    binary_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -2;
    }
    
    if (header.magic != BINARY_MAGIC) {
        close(fd);
        return -3;
    }
    
    void* program_memory = kmalloc(header.code_size + header.data_size + header.bss_size);
    if (!program_memory) {
        close(fd);
        return -4;
    }
    
    if (read(fd, program_memory, header.code_size + header.data_size) != 
            (header.code_size + header.data_size)) {
        kfree(program_memory);
        close(fd);
        return -5;
    }
    
    memset((uint8_t*)program_memory + header.code_size + header.data_size, 0, header.bss_size);
    
    *entry_point = (uint64_t)program_memory + header.entry_point;
    close(fd);
    return 0;
}