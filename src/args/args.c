#include <utils/str_utils.h>
#include <utils/def_utils.h>
#include "args.h"

char *args_to_string(const args *args) {
    char *options_str = strjoin((char **)args->options->data, args->options->len, ", ");
    char * s = strmake(
        "input = %s\n"
        "output = %s\n"
        "verbosity = %d\n"
        "method = %s\n"
        "draw_directory = %s\n"
        "draw_overview_file = %s\n"
        "force_draw = %s\n"
        "write_lp = %s\n"
        "time_limit = %d\n"
        "num_threads = %d\n"
        "seed = %u\n"
        "assignments_difficulty_ranking_randomness = %g\n"
        "multistart = %d\n"
        "config = %s\n"
        "options = %s",
             args->input,
             args->output,
             args->verbosity,
             resolution_method_to_string(args->method),
             args->draw_directory,
             args->draw_overview_file,
             BOOL_TO_STR(args->force_draw),
             args->write_lp_file,
             args->time_limit,
             args->num_threads,
             args->seed,
             args->assignments_difficulty_ranking_randomness,
             args->multistart,
             args->config,
             options_str
    );

    free(options_str);

    return s;
}

void args_init(args *args) {
    args->input = NULL;
    args->output = NULL;
    args->verbosity = ARG_INT_NONE;
    args->method = RESOLUTION_METHOD_DEFAULT;
    args->draw_directory = NULL;
    args->draw_overview_file = NULL;
    args->force_draw = false;
    args->write_lp_file = NULL;
    args->solution_input_file = NULL;
    args->time_limit = ARG_INT_NONE;
    args->num_threads = ARG_INT_NONE;
    args->seed = ARG_INT_NONE;
    args->assignments_difficulty_ranking_randomness = ARG_INT_NONE;
    args->multistart = ARG_INT_NONE;
    args->config = NULL;
    args->options = g_array_new(false, false, sizeof(char *));
}

void args_destroy(args *args) {
    g_array_free(args->options, false);
}