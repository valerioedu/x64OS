#ifndef R0IO_H
#define R0IO_H

#include "definitions.h"

int scanf(const char* format, ...);
char* kfgets(char* buffer, int size, int fd);
int sprintf(char* buffer, const char* format, ...);

#endif