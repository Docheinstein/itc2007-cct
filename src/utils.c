#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
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

    char *s = malloc(sizeof(char) * buflen);
    s[0] = '\0';

    for (int i = 0; i < size - 1; i++)
        strappendf(s, buflen, "%s%s", strs[i], joiner);
    strappendf(s, buflen, "%s", strs[size - 1]);       // last, without joiner

    return s; // must be freed outside
}
