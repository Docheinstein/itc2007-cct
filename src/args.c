#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include "args.h"
#include "utils/str_utils.h"
#include "utils/io_utils.h"
#include "utils/def_utils.h"
#include <stdbool.h>
#include "config.h"

void args_to_string(const args *args, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
        "input = %s\n"
        "output = %s\n"
        "verbose = %s\n"
        "method = %s\n"
        "draw_directory = %s\n"
        "draw_overview_file = %s\n"
        "force_draw = %s\n"
        "write_lp = %s\n"
        "time_limit = %d\n"
        "seed = %u\n"
        "assignments_difficulty_ranking_randomness = %g\n"
        "multistart = %d",
             args->input,
             args->output,
             BOOL_TO_STR(args->verbose),
             resolution_method_to_string(args->method),
             args->draw_directory,
             args->draw_overview_file,
             BOOL_TO_STR(args->force_draw),
             args->write_lp_file,
             args->time_limit,
             args->seed,
             args->assignments_difficulty_ranking_randomness,
             args->multistart
    );
}

void args_init(args *args) {
    args->input = NULL;
    args->output = NULL;
    args->verbose = false;
    args->method = RESOLUTION_METHOD_DEFAULT;
    args->draw_directory = NULL;
    args->draw_overview_file = NULL;
    args->force_draw = false;
    args->write_lp_file = NULL;
    args->solution_input_file = NULL;
    args->time_limit = ARG_INT_NONE;
    args->seed = ARG_INT_NONE;
    args->assignments_difficulty_ranking_randomness = ARG_INT_NONE;
    args->multistart = ARG_INT_NONE;
}

void args_destroy(args *args) {

}

const char *argp_program_version = "0.1";
static const char *doc = "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007";
static const char *args_doc = "INPUT [OUTPUT]";

typedef enum itc2007_option {
    OPTION_VERBOSE = 'v',
    OPTION_METHOD = 'm',
    OPTION_TIME_LIMIT = 't',
    OPTION_DRAW_DIRECTORY = 'D',
    OPTION_DRAW_OVERVIEW_FILE = 'd',
    OPTION_FORCE_DRAW = 'f',
    OPTION_SOLUTION = 'S',
    OPTION_SEED = 's',
    OPTION_ASSIGNMENTS_DIFFICULTY_RANKING_RANDOMNESS = 'r',
    OPTION_MULTISTART = 'n',
    OPTION_WRITE_LP = 0x100,
} itc2007_option;


static struct argp_option options[] = {
  { "verbose", OPTION_VERBOSE, NULL, 0,
        "Print debugging information" },
  { "write-lp", OPTION_WRITE_LP, "FILE", 0,
        "Write .lp to file (only for 'exact' method)" },
  { "draw-all", OPTION_DRAW_DIRECTORY, "DIR", 0,
        "Draw all the timetables of the solution and place them in DIR" },
  { "draw-overview", OPTION_DRAW_OVERVIEW_FILE, "FILE", 0,
        "Draw the overview timetable of the solution and save it as FILE" },
  { "force-draw", OPTION_FORCE_DRAW, NULL, 0,
        "Draw even if a feasible solution can't be provided" },
  { "time", OPTION_TIME_LIMIT, "SECONDS", 0,
        "Time limit in seconds for solve the model" },
  { "solution", OPTION_SOLUTION, "SOL_FILE", 0,
        "Load the solution file SOL_FILE instead of computing it"
        "(useful for see the cost/violations or with -d or -D for render the solution)" },
  { "method", OPTION_METHOD, "exact|tabu", 0,
        "Method to use for solve the model.\n"
        "Possible values are: 'exact'" },
  { "seed", OPTION_SEED, "N", 0,
        "Seed to use for randomization, if not provided a random one is used" },
  { "ranking-randomness", OPTION_ASSIGNMENTS_DIFFICULTY_RANKING_RANDOMNESS, "N", 0,
        "Factor used for rank the lectures to assign while finding an "
        "initial feasible solution (only for heuristics method).\n"
        "Default is 0.33.\n"
        "If is 0 is given, it will generate always the same deterministic solution." },
  { "multistart", OPTION_MULTISTART, "N", 0,
        "Number of run for heuristics methods; "
        "the final solution will be the best among the runs" },
  { NULL }
};

static int parse_int(const char *str) {
    bool ok;
    int val = strtoint(str, &ok);
    if (!ok) {
        eprint("ERROR: integer conversion failed for parameter '%s'", str);
        exit(EXIT_FAILURE);
    }
    return val;
}

static uint parse_uint(const char *str) {
    bool ok;
    uint val = strtouint(str, &ok);
    if (!ok) {
        eprint("ERROR: integer conversion failed for parameter '%s'", str);
        exit(EXIT_FAILURE);
    }
    return val;
}

static double parse_double(const char *str) {
    bool ok;
    double val = strtodouble(str, &ok);
    if (!ok) {
        eprint("ERROR: integer conversion failed for parameter '%s'", str);
        exit(EXIT_FAILURE);
    }
    return val;
}

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch (key) {
    case OPTION_VERBOSE:
        args->verbose = true;
        break;
    case OPTION_DRAW_DIRECTORY:
        args->draw_directory = arg;
        break;
    case OPTION_DRAW_OVERVIEW_FILE:
        args->draw_overview_file = arg;
        break;
    case OPTION_FORCE_DRAW:
        args->force_draw = true;
        break;
    case OPTION_SOLUTION:
        args->solution_input_file = arg;
        break;
    case OPTION_WRITE_LP:
        args->write_lp_file = arg;
        break;
    case OPTION_TIME_LIMIT:
        args->time_limit = parse_int(arg);
        break;
    case OPTION_METHOD:
        if (streq(arg, "exact"))
            args->method = RESOLUTION_METHOD_EXACT;
        else if (streq(arg, "tabu"))
            args->method = RESOLUTION_METHOD_TABU;
        else {
            eprint("ERROR: unknown method '%s'.\n"
                   "Possible values are 'exact', 'tabu'", arg);
            exit(EXIT_FAILURE);
        }
        break;
    case OPTION_SEED:
        args->seed = parse_uint(arg);
        break;
    case OPTION_ASSIGNMENTS_DIFFICULTY_RANKING_RANDOMNESS:
        args->assignments_difficulty_ranking_randomness = parse_double(arg);
        break;
    case OPTION_MULTISTART:
        args->multistart = parse_int(arg);
        break;
    case ARGP_KEY_ARG:
        switch (state->arg_num) {
        case 0:
            args->input = arg;
            break;
        case 1:
            args->output = arg;
            break;
        default:
            eprint("ERROR: Unexpected argument '%s'", arg);
            argp_usage(state);
        }

      break;
    case ARGP_KEY_END:
        if (state->arg_num < 1)
            argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

void args_parse(args *args, int argc, char **argv) {
    struct argp argp = {options, parse_option, args_doc, doc};
    argp_parse(&argp, argc, argv, 0, 0, args);
}