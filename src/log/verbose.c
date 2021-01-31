#include "verbose.h"
#include <stdarg.h>
#include <stdio.h>

static int verbosity;

int get_verbosity() {
    return verbosity;
}

void set_verbosity(int level) {
    verbosity = level;
}

void verbose_lv(int level, const char *fmt, ...) {
#ifndef DEBUG
    // In DEBUG mode, always show verbose messages
    if (level > verbosity)
        return;
#endif

    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
}
