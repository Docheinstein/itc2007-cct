#include "assert_utils.h"
#include <stdlib.h>
#include "utils/io_utils.h"

void assertion_message(const char *file, int line, const char *func, const char *message) {
    eprint("ASSERT FAILED %s:%d:%s %s", file, line, func, message);
    exit(EXIT_FAILURE);
}
