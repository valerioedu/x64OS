#ifndef COMMANDS_H
#define COMMANDS_H

#include "../../lib/definitions.h"

typedef struct {
    char* name;
    void (*function)(char*);
} Command;

void help(char* args);
void clear(char* args);
void mkdir(char* args);
void cd(char* args);
void ls(char* args);
void touch(char* args);
void rm(char* args);
void rmdir(char* args);

#endif