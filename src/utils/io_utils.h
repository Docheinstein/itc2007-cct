#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdio.h>

#define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define fprint(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)


char *fileread(const char *filename);
int filewrite(const char *filename, const char *data);

#endif // IO_UTILS_H