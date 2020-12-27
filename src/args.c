#include <stdio.h>
#include "args.h"

int args_dump(args *args, char *dump, int size) {
    return snprintf(dump, size,
       "input = %s\n"
       "output = %s\n"
       "verbose = %d"
       ,
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