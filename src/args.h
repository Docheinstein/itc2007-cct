#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

typedef enum itc2007_method {
    ITC2007_METHOD_EXACT,
    ITC2007_METHOD_TABU,
    ITC2007_METHOD_DEFAULT = ITC2007_METHOD_EXACT
} itc2007_method;

typedef struct args {
    char *input;
    char *output;
    bool verbose;
    itc2007_method method;
    char *write_lp_file;
    char *solution_input_file;
    char *draw_directory;
    char *draw_overview_file;
    int time_limit;
} args;

void args_parse(args *args, int argc, char **argv);
void args_to_string(const args *args, char *buffer, size_t buflen);
void args_init(args *args);
void args_destroy(args *args);


#endif // ARGS_H