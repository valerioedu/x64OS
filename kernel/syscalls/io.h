#ifndef IO_H
#define IO_H

#include "../../lib/definitions.h"
#include "sys_read/read.h"

#ifndef stdout
#define stdout 1
#endif

#ifndef stdin
#define stdin 0
#endif

#ifndef stderr
#define stderr 2
#endif

typedef struct io{
    int fd;
    uint32_t buf_len;
    uint8_t* buf;
    uint8_t* end;
    uint8_t* data;
    bool eof;
} io;

#endif