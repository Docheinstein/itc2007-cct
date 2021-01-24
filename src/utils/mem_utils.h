#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#include <stddef.h>

#define freearray(ptr, nmemb) do { \
    if (ptr) {                     \
        for (int i = 0; i < (nmemb); i++) \
        free((ptr)[i]); \
    } \
    free(ptr); \
} while(0)

void *mallocx(size_t nmemb, size_t size);
void *callocx(size_t nmemb, size_t size);

#endif // MEM_UTILS_H