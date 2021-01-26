#ifndef ARGS_H
#define ARGS_H

#include <glib.h>
#include <resolution_method.h>
#include <stdbool.h>

static const int ARG_INT_NONE = -1;

typedef struct args {
    char *input;
    char *output;
    int verbosity;
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
    char *config;
    GArray *options;
    bool benchmark_mode;
} args;

void args_init(args *args);
void args_destroy(args *args);

char *args_to_string(const args *args);

#endif // ARGS_H