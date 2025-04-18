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

extern syscall_t syscall_table[];

#endif