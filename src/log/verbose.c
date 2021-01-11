#include <stdarg.h>
#include <stdio.h>
#include "verbose.h"

static int is_verbose_;

bool is_verbose() {
    return is_verbose_;
}

void set_verbose(bool yes) {
    is_verbose_ = yes;
}

int verbosef(const char *fmt, ...) {
    if (!is_verbose_)
        return 0;

    va_list args;
    va_start(args, fmt);

    int ret = vprintf(fmt, args);

    va_end(args);

    return ret;
}
