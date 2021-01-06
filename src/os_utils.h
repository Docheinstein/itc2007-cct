#include <stdbool.h>

#ifndef OS_UTILS_H
#define OS_UTILS_H

#ifdef _WIN32
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

char * pathjoin_internal(const char *first, ...);
#define pathjoin(...) pathjoin_internal(__VA_ARGS__, NULL)

bool exists(const char *path);
bool isfile(const char *path);
bool isdir(const char *path);

bool mkdirs(const char *path);

#endif // OS_UTILS_H