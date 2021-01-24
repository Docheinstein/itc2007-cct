#ifndef VERBOSE_H
#define VERBOSE_H

#include <stdbool.h>

int get_verbosity();
void set_verbosity(int level);

int verbose_internal(int level, const char *fmt, ...);

#define verbose(format, ...) verbose_internal(1, format "\n", ##__VA_ARGS__)
#define verbose2(format, ...) verbose_internal(2, format "\n", ##__VA_ARGS__)

#endif // VERBOSE_H"