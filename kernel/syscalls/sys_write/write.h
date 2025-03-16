#ifndef WRITE_H
#define WRITE_H

#include "../../../lib/definitions.h"

#ifndef stdout
#define stdout 1
#endif

ssize_t write(int fd, const void* buf, size_t nbyte);

#endif