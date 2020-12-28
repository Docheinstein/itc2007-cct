#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#define strappend(dest, size, str) do { \
    const int _len = strlen(dest); \
    snprintf(&dest[_len], (int) (size - _len), "%s", str); } while(0);

#define strappendf(dest, size, fmt, ...) do { \
    const int _len = strlen(dest); \
    snprintf(&dest[_len], (int) (size - _len), fmt, ##__VA_ARGS__); } while(0);

bool streq(const char *s1, const char *s2);
bool strempty(const char *str);
int strpos(const char *str, char character);

char * strrtrim(char *str);
char * strltrim(char *str);
char * strtrim(char *str);

int strtoint(const char *str, bool *ok);

int strsplit(char *str, const char *delimiters, char **tokens, size_t size);

#endif // UTILS_H