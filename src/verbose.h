#ifndef VERBOSE_H
#define VERBOSE_H

#include <stdbool.h>

static is_verbose;
void set_verbose(bool yes);

int verbose(const char *fmt, ...);

#endif // VERBOSE_H