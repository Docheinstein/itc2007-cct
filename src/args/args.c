#include <utils/str_utils.h>
#include "args.h"

char *args_to_string(const args *args) {
    char *options_str = strjoin((char **)args->options->data, args->options->len, ", ");
    char * s = strmake(
        "input = %s\n"
        "output = %s\n"
        "verbosity = %d\n"
        "seed = %u\n"
        "max_time = %d\n"
        "config = %s\n"
        "options = %s\n"
        "benchmark_mode = %s\n"
        "dont_solve = %s\n"
        "draw_all_directory = %s\n"
        "draw_overview_file = %s\n"
        "input_solution = %s",
        args->input_file,
        args->output_file,
        args->verbosity,
        args->seed,
        args->max_time,
        args->config_file,
        options_str,
        booltostr(args->benchmark_mode),
        booltostr(args->dont_solve),
        args->draw_all_directory,
        args->draw_overview_file,
        args->solution_input_file
    );

    free(options_str);

    return s;
}

void args_init(args *args) {
    args->input_file = NULL;
    args->output_file = NULL;
    args->verbosity = 0;
    args->seed = 0;
    args->max_time = 0;
    args->config_file = NULL;
    args->options = g_array_new(false, false, sizeof(char *));
    args->benchmark_mode = false;
    args->dont_solve = false;
    args->draw_all_directory = NULL;
    args->draw_overview_file = NULL;
    args->solution_input_file = NULL;
}

void args_destroy(args *args) {
    g_array_free(args->options, false);
}