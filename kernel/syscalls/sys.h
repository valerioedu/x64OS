#ifndef SYS_H
#define SYS_H

#include "sys_read/read.h"
#include "sys_write/write.h"
#include "sys_open/open.h"
#include "sys_close/close.h"
#include "sys_stat/stat.h"
#include "sys_sleep/sleep.h"

typedef struct syscall_t {
    int syscall_no;
    void* syscall;
} syscall_t;

syscall_t syscall_table[] = {
    {0, &read},
    {1, &write},
    {2, &open},
    {3, &close},
    {4, &stat},
    {5, &fstat},
    {6, &sleep}
};

#endif