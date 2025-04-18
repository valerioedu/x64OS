#include "sys.h"

syscall_t syscall_table[] = {
    {0, &read},
    {1, &write},
    {2, &open},
    {3, &close},
    {4, &stat},
    {5, &fstat},
    {6, &sleep}
};