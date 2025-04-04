#ifndef READ_H
#define READ_H

#include "../../../lib/definitions.h"
#include "../io.h"

ssize_t read(int fd, void* buf, size_t count);

#endif