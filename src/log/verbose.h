#ifndef VERBOSE_H
#define VERBOSE_H

/*
 * Verbose statements.
 * Enabled only if the verbosity level set with set_verbosity
 * is greater than the message level)
 */

void set_verbosity(int level);
int get_verbosity();

void verbose_lv(int level, const char *fmt, ...);

#define verbose(format, ...) verbose_lv(1, format "\n", ##__VA_ARGS__)
#define verbosef(format, ...) verbose_lv(1, format, ##__VA_ARGS__)

#define verbose2(format, ...) verbose_lv(2, format "\n", ##__VA_ARGS__)
#define verbosef2(format, ...) verbose_lv(2, format, ##__VA_ARGS__)

#endif // VERBOSE_H"