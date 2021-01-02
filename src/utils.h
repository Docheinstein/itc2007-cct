#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#define strappend(dest, size, fmt, ...) do { \
    const int _len = strlen(dest); \
    snprintf(&(dest)[_len], (int) ((size) - _len), fmt, ##__VA_ARGS__); } while(0);

#define vstrappend(dest, size, fmt, args) do { \
    const int _len = strlen(dest); \
    vsnprintf(&(dest)[_len], (int) ((size) - _len), fmt, args); } while(0);

bool streq(const char *s1, const char *s2);
bool strempty(const char *str);
int strpos(const char *str, char character);

char * strrtrim(char *str);
char * strltrim(char *str);
char * strtrim(char *str);

int strtoint(const char *str, bool *ok);

int strsplit(char *str, const char *delimiters, char **tokens, size_t max_tokens);
char * strjoin(char **strs, size_t size, const char *joiner);

//char * strmake(const char * fmt, ...);

void strappend_realloc(char **dest, size_t *size, const char *fmt, ...);

void *mallocx(size_t size);

char *fileread(const char *filename);
int filewrite(const char *filename, const char *data);

#endif // UTILS_H