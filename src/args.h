#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

typedef struct args {
    char *input;
    char *output;
    bool verbose;
} args;

void args_dump(args *args, char *dump, int size);
void args_dump_indent(args *args, char *dump, int size, const char *indent);
void args_init(args *args);


#endif // ARGS_H