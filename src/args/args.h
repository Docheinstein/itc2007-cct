#ifndef ARGS_H
#define ARGS_H

#include <glib.h>
#include <heuristics/methods/heuristic_method.h>
#include <stdbool.h>

typedef struct args {
    char *input_file;
    char *output_file;
    int verbosity;
    unsigned int seed;
    int max_time;
    char *config_file;
    GArray *options;
    bool benchmark_mode;
    bool dont_solve;
    char *draw_all_directory;
    char *draw_overview_file;
    char *solution_input_file;
} args;

void args_init(args *args);
void args_destroy(args *args);

char *args_to_string(const args *args);

#endif // ARGS_H