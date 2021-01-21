#include "common_utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void fail(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
