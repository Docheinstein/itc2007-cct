#include "str_utils.h"
#include "mem_utils.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

bool streq(const char *s1, const char *s2) {
    return (!s1 && !s2) || s1 && s2 && strcmp(s1, s2) == 0;
}

bool strempty(const char *str) {
    return !str || strcmp(str, "") == 0;
}

int strpos(const char *str, char character) {
    const char *c = strchr(str, character);

    if (c == NULL)
        return -1;

    return (int) (c - str);
}

char *strrtrim(char *str) {
    char *end = &str[strlen(str) - 1];

    while (isspace(*end) && end >= str)
        --end;

    end[1] = '\0';

    return str;
}

char *strltrim(char *str) {
    char *start = str;

    while (isspace(*start))
        ++start;

    return start;
}

char *strtrim(char *str) {
    char *start = strltrim(str);
    return strrtrim(start);
}


char *strrtrim_chars(char *str, const char *chars) {
    char *end = &str[strlen(str) - 1];

    do {
        bool go_ahead = false;

        for (int i = 0; i < strlen(chars); i++) {
            if (*end == chars[i] && end >= str) {
                --end;
                go_ahead = true;
                break;
            }
        }

        if (!go_ahead)
            break;
    } while (true);

    end[1] = '\0';

    return str;
}

char *strltrim_chars(char *str, const char *chars) {
    char *start = str;

    do {
        bool go_ahead = false;

        for (int i = 0; i < strlen(chars); i++) {
            if (*start == chars[i]) {
                ++start;
                go_ahead = true;
                break;
            }
        }

        if (!go_ahead)
            break;
    } while (true);

    return start;
}

char *strtrim_chars(char *str, const char *chars) {
    char *start = strltrim_chars(str, chars);
    return strrtrim_chars(start, chars);
}


int strtoint(const char *str, bool *ok) {
    return (uint) strtolong(str, ok);
}

uint strtouint(const char *str, bool *ok) {
    return (uint) strtoulong(str, ok);
}

long strtolong(const char *str, bool *ok) {
    char *endptr = NULL;

    errno = 0;
    long ret = strtol(str, &endptr, 10);

    if (ok != NULL)
        *ok = (errno == 0 && str && endptr && *endptr == '\0');

    return ret;
}

ulong strtoulong(const char *str, bool *ok) {
    char *endptr = NULL;

    errno = 0;
    unsigned long ret = strtoul(str, &endptr, 10);

    if (ok != NULL)
        *ok = (errno == 0 && str && endptr && *endptr == '\0');

    return ret;
}


int strsplit(char *str, const char *delimiters, char **tokens, size_t max_tokens) {
    if (max_tokens <= 1)
        return 0;

    int i = 0;
    char *token = strtok(str, delimiters);

    if (!token)
        return 0;

    do {
        if (!strempty(token))
            tokens[i++] = token;
    } while ((token = strtok(NULL, delimiters)) && i < max_tokens);

    return i;
}

char *strjoin(char **strs, size_t size, const char *joiner) {
    const int joiner_len = (int) strlen(joiner);
    int buflen = 0;

    for (int i = 0; i < size - 1; i++)
        buflen += (int) strlen(strs[i]) + joiner_len;
    buflen += (int) strlen(strs[size - 1]);            // last, without joiner
    buflen += 1;                                       // '\0'

    char *s = mallocx(sizeof(char) * buflen);
    s[0] = '\0';

    for (int i = 0; i < size - 1; i++)
        strappend(s, buflen, "%s%s", strs[i], joiner);
    strappend(s, buflen, "%s", strs[size - 1]);       // last, without joiner

    return s; // must be freed outside
}


char *strmake(const char *fmt, ...) {
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);

    int buflen = vsnprintf(NULL, 0, fmt, args) + 1;

    char *s = mallocx(sizeof(char) * buflen);
    vsnprintf(s, buflen, fmt, args2);

    va_end(args);
    va_end(args2);

    return s;
}

void strappend_realloc(char **dest, size_t *size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vstrappend_realloc(dest, size, fmt, args);
    va_end(args);
}

void strappend(char *dest, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vstrappend(dest, size, fmt, args);
    va_end(args);
}

void vstrappend(char *dest, size_t size, const char *fmt, va_list args) {
    const size_t _len = strlen(dest);
    vsnprintf(&dest[_len], (int) ((size) - _len), fmt, args);
}

void vstrappend_realloc(char **dest, size_t *size, const char *fmt, va_list args) {
    if (!dest)
        return;

    static const size_t DEFAULT_BUFFER_SIZE = 256;

    if (!*dest) {
        *size = DEFAULT_BUFFER_SIZE;
        *dest = mallocx(DEFAULT_BUFFER_SIZE);
        *dest[0] = '\0';
    }

    va_list args_copy;
    va_copy(args_copy, args);

    const size_t required_size =
            strlen(*dest) + vsnprintf(NULL, 0, fmt, args) + 1;

    if (*size < required_size) {
        while (*size < required_size)
            *size *= 2; // grow buffer length exponentially for avoid realloc

        *dest = realloc(*dest, *size);
    }

    vstrappend(*dest, *size, fmt, args_copy);

    va_end(args_copy);
}

bool strstarts(const char *str, char ch) {
    return strfirst(str) == ch;
}

bool strends(const char *str, char ch) {
    return strlast(str) == ch;
}

char strfirst(const char *str) {
    return str[0];
}

char strlast(const char *str) {
    return str[strlen(str) - 1];
}