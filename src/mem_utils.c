#include "mem_utils.h"
#include "io_utils.h"
#include <stdlib.h>

void *mallocx(size_t nmemb, size_t size) {
    void *ptr = malloc(nmemb * size);
    if (!ptr) {
        eprint("ERROR: malloc failed (out of memory)");
        exit(-1);
    }
    return ptr;
}

void *callocx(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        eprint("ERROR: calloc failed (out of memory)");
        exit(-1);
    }
    return ptr;
}
