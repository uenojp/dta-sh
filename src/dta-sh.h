#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// ref. https://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html
#ifdef DEBUG_PRINT
#define DEBUG(format, ...)                                           \
    do {                                                             \
        fprintf(stderr, "[pid %d] %-16s: ", getpid(), __FUNCTION__); \
        fprintf(stderr, format, ##__VA_ARGS__);                      \
        fprintf(stderr, "\n");                                       \
    } while (0)
#else
#define DEBUG(format, ...)
#endif
