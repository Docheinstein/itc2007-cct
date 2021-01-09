#include "os_utils.h"
#include <stdarg.h>
#include <sys/stat.h>
#include "str_utils.h"
#include "debug.h"

char *pathjoin0(const char *first, ...) {
    va_list args;
    va_start(args, first);

    char * buffer = NULL;
    size_t size;
    strappend_realloc(&buffer, &size, "%s", first);

    char *arg = NULL;

    debug("pathjoin: %s", buffer);

    while (true) {
        arg = va_arg(args, char *);
        if (!arg)
            break;

        strrtrim_chars(buffer, PATH_SEP_STR);
        strappend_realloc(&buffer, &size,
                          "%s%s",
                          (strstarts(arg, PATH_SEP)) ? "" : PATH_SEP_STR,
                          arg);

        debug("pathjoin: %s", buffer);
    }

    va_end(args);

    return buffer;
}

bool exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool isfile(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

bool isdir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool mkdirs(const char *path) {
    debug("mkdir: %s", path);
    if (exists(path))
        return true;
    return mkdir(path, 0755) == 0;
}
