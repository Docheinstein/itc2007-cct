#include "utils.h"
#include <string.h>

long strpos(const char *str, char character) {
    const char *c = strchr(str, character);
    if (c == NULL)
        return -1;
    return c - str;
}
