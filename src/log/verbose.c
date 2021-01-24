#include <stdarg.h>
#include <stdio.h>
#include "verbose.h"

static int verbosity;

int get_verbosity() {
    return verbosity;
}

void set_verbosity(int level) {
    verbosity = level;
}

int verbose_internal(int level, const char *fmt, ...) {
    if (level > verbosity)
        return 0;

    va_list args;
    va_start(args, fmt);

    int ret = vprintf(fmt, args);

    va_end(args);

    return ret;
}
