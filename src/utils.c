#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include "log/debug.h"

bool streq(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

bool strempty(const char *str) {
    return strcmp(str, "") == 0;
}

int strpos(const char *str, char character) {
    const char *c = strchr(str, character);

    if (c == NULL)
        return -1;

    return (int) (c - str);
}

char *strrtrim(char *str) {
    char *end = &str[strlen(str) - 1];

    while (isspace(*end) && end > str)
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
    strrtrim(start);
    return start;
}

int strtoint(const char *str, bool *ok) {
    char *endptr = NULL;

    errno = 0;
    int ret = (int) strtol(str, &endptr, 10);

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

/*
char *strmake(const char *fmt, ...) {
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);

    int buflen = snprintf(NULL, 0, fmt, args) + 1;

    char *s = mallocx(sizeof(char) * buflen);
    snprintf(s, buflen, fmt, args2);

    va_end(args);
    va_end(args2);

    return s;
}
*/

void strappend_realloc(char **dest, size_t *size, const char *fmt, ...) {
    if (!dest)
        return;

    static const size_t DEFAULT_BUFFER_SIZE = 256;

    if (!*dest) {
        *size = DEFAULT_BUFFER_SIZE;
        *dest = mallocx(DEFAULT_BUFFER_SIZE);
        *dest[0] = '\0';
    }

    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);

    const size_t required_size =
            strlen(*dest) + vsnprintf(NULL, 0, fmt, args) + 1;

    if (*size < required_size) {
        while (*size < required_size)
            *size *= 2; // grow buffer length exponentially for avoid realloc

        *dest = realloc(*dest, *size);
    }

    vstrappend(*dest, *size, fmt, args2);

    va_end(args);
    va_end(args2);
}

char *fileread(const char *filename) {
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = mallocx(filesize + 1);
    fread(content, sizeof(char), filesize, f);
    fclose(f);

    content[filesize] = '\0';
    return content; // must be freed outside
}

void *mallocx(size_t size) {
    void *ptr = malloc(size);
    if (!ptr)
        eprint("ERROR: malloc failed");
    return ptr;
}
