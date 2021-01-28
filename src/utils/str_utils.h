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

char * strrtrim_chars(char *str, const char *chars);
char * strltrim_chars(char *str, const char *chars);
char * strtrim_chars(char *str, const char *chars);

bool strtoint(const char *str, int *value);
bool strtouint(const char *str, unsigned int *value);
bool strtolong(const char *str, long *value);
bool strtoulong(const char *str, unsigned long *value);
bool strtodouble(const char *str, double *value);
bool strtobool(const char *str, bool *value);
const char * booltostr(bool value);

int strsplit(char *str, const char *delimiters, char **tokens, size_t max_tokens);
char * strjoin(char **strs, size_t size, const char *joiner);

bool strstarts(const char *str, char ch);
bool strends(const char *str, char ch);

char strfirst(const char *str);
char strlast(const char *str);



#endif // STR_UTILS_H