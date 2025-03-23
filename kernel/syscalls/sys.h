#ifndef SYS_H
#define SYS_H

#include "sys_read/read.h"
#include "sys_write/write.h"

typedef struct syscall_t {
    int syscall_no;
    void* syscall;
} syscall_t;

syscall_t syscall_table[] = {
    {0, &read},
    {1, &write}
};

#endif