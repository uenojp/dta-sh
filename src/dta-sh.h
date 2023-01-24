#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG(...)                        \
    do {                                  \
        debug(__FUNCTION__, __VA_ARGS__); \
    } while (0);

void debug(const char* function_name, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "[pid %d] %-16s: ", getpid(), function_name);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
}
