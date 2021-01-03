#include "mem_utils.h"
#include "io_utils.h"
#include <stdlib.h>

void *mallocx(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        eprint("ERROR: malloc failed (out of memory)");
        exit(-1);
    }
    return ptr;
}
