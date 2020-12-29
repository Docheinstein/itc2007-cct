#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

typedef struct args {
    char *input;
    char *output;
    bool verbose;
} args;

void args_to_string(const args *args, char *buffer, size_t buflen);
void args_init(args *args);


#endif // ARGS_H