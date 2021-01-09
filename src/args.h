#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include "resolution_method.h"

#define ARG_INT_NONE (-1)

typedef struct args {
    char *input;
    char *output;
    bool verbose;
    resolution_method method;
    char *write_lp_file;
    char *solution_input_file;
    char *draw_directory;
    char *draw_overview_file;
    bool force_draw;
    int time_limit;
    uint seed;
    double assignments_difficulty_ranking_randomness;
    int multistart;
} args;

void args_parse(args *args, int argc, char **argv);
void args_to_string(const args *args, char *buffer, size_t buflen);
void args_init(args *args);
void args_destroy(args *args);


#endif // ARGS_H