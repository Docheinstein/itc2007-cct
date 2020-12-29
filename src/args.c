#include <stdio.h>
#include "args.h"

void args_to_string(const args *args, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
        "input = %s\n"
        "output = %s\n"
        "verbose = %d",
        args->input,
        args->output,
        args->verbose
    );
}

void args_init(args *args) {
    args->input = NULL;
    args->output = NULL;
    args->verbose = false;
}
