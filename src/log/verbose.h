#ifndef VERBOSE_H
#define VERBOSE_H

#include <stdbool.h>

static int is_verbose;
void set_verbose(bool yes);

int verbosef(const char *fmt, ...);

#define verbose(format, ...) verbosef(format "\n", ##__VA_ARGS__)

#endif // VERBOSE_H