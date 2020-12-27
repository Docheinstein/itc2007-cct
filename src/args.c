#include <stdio.h>
#include "args.h"


int args_dump(args *args, char *dump, int size) {
    return args_dump_indent(args, dump, size, "");
}

int args_dump_indent(args *args, char *dump, int size, const char *indent) {
    return snprintf(dump, size,
       "%sinput = %s\n"
       "%soutput = %s\n"
       "%sverbose = %d"
       ,
       indent, args->input,
       indent, args->output,
       indent, args->verbose
   );
}

void args_init(args *args) {
    args->input = NULL;
    args->output = NULL;
    args->verbose = false;
}
