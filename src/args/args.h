#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <glib.h>

/* Arguments given at the command line. */

typedef struct args {
    // Positional
    char *input_file;
    char *output_file;

    // Options
    int verbosity;
    unsigned int seed;
    int max_time;
    char *config_file;
    GArray *options;
    bool benchmark_mode;
    char *benchmark_file;
    bool race_mode;
    bool quiet;
    bool dont_solve;
    int print_violations;
    char *draw_all_directory;
    char *draw_overview_file;
    char *solution_input_file;
} args;

void args_init(args *args);
void args_destroy(args *args);

char *args_to_string(const args *args);

#endif // ARGS_H