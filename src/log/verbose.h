#ifndef VERBOSE_H
#define VERBOSE_H

#include <stdbool.h>

int get_verbosity();
void set_verbosity(int level);

void verbose_lv(int level, const char *fmt, ...);

#define verbose(format, ...) verbose_lv(1, format "\n", ##__VA_ARGS__)
#define verbosef(format, ...) verbose_lv(1, format, ##__VA_ARGS__)

#define verbose2(format, ...) verbose_lv(2, format "\n", ##__VA_ARGS__)
#define verbosef2(format, ...) verbose_lv(2, format, ##__VA_ARGS__)

#endif // VERBOSE_H"