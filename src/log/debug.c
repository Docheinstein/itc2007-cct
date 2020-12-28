#ifdef DEBUG

#include "debug.h"
#include <stdarg.h>
#include <stdio.h>

int debugf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int ret = vprintf(fmt, args);

    va_end(args);

    return ret;
}

#endif