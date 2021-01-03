#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

void strappend(char *dest, size_t size, const char *fmt, ...);
void strappend_realloc(char **dest, size_t *size, const char *fmt, ...);

void vstrappend(char *dest, size_t size, const char *fmt, va_list args);
void vstrappend_realloc(char **dest, size_t *size, const char *fmt, va_list args);

char * strmake(const char * fmt, ...);

bool streq(const char *s1, const char *s2);
bool strempty(const char *str);
int strpos(const char *str, char character);

char * strrtrim(char *str);
char * strltrim(char *str);
char * strtrim(char *str);

int strtoint(const char *str, bool *ok);

int strsplit(char *str, const char *delimiters, char **tokens, size_t max_tokens);
char * strjoin(char **strs, size_t size, const char *joiner);



#endif // STR_UTILS_H