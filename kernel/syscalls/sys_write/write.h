#ifndef WRITE_H
#define WRITE_H

#include "../../../lib/definitions.h"
#include "../io.h"

ssize_t write(int fd, const void* buf, size_t nbyte);

#endif