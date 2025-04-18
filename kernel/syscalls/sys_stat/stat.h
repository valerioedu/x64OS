#ifndef STAT_H
#define STAT_H

#include "../../../lib/definitions.h"
#include "../../fs/src/file.h"

int stat(const char* pathname, struct stat* statbuf);
int fstat(int fd, struct stat* statbuf);

#endif