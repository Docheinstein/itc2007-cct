#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

bool streq(const char *s1, const char *s2);
int strpos(const char *str, char character);

char * strrtrim(char *str);
char * strltrim(char *str);
char * strtrim(char *str);

int strtoint(const char *str, bool *ok);

#endif // UTILS_H