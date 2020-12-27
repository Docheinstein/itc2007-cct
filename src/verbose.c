#include <stdarg.h>
#include <stdio.h>
#include "verbose.h"

void set_verbose(bool yes) {
    is_verbose = yes;
}

int verbose(const char *fmt, ...) {
    if (!is_verbose)
        return 0;

    va_list args;
    va_start(args, fmt);

    int ret = vprintf(fmt, args);

    va_end(args);

    return ret;
}
