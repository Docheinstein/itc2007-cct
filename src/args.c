#include <stdio.h>
#include "args.h"


void args_dump(args *args, char *dump, int size) {
    args_dump_indent(args, dump, size, "");
}

void args_dump_indent(args *args, char *dump, int size, const char *indent) {
    snprintf(dump, size,
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
