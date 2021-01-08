#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include "args.h"
#include "str_utils.h"
#include "io_utils.h"
#include "def_utils.h"
#include <stdbool.h>


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
        "seed = %u",
             args->input,
             args->output,
             BOOL_TO_STR(args->verbose),
             resolution_method_to_string(args->method),
             args->draw_directory,
             args->draw_overview_file,
             BOOL_TO_STR(args->force_draw),
             args->write_lp_file,
             args->time_limit,
             args->seed
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
    args->time_limit = 0;
    args->seed = 0;
}

void args_destroy(args *args) {

}

const char *argp_program_version = "0.1";
static const char *doc = "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007";
static const char *args_doc = "INPUT [OUTPUT]";

typedef enum itc2007_option {
    ITC2007_OPT_VERBOSE = 'v',
    ITC2007_OPT_METHOD = 'm',
    ITC2007_OPT_TIME_LIMIT = 't',
    ITC2007_OPT_DRAW_DIRECTORY = 'D',
    ITC2007_OPT_DRAW_OVERVIEW_FILE = 'd',
    ITC2007_OPT_FORCE_DRAW = 'f',
    ITC2007_OPT_SOLUTION = 's',
    ITC2007_OPT_SEED = 'S',
    ITC2007_OPT_WRITE_LP = 0x100,
} itc2007_option;


static struct argp_option options[] = {
  { "verbose", ITC2007_OPT_VERBOSE, NULL, 0, "Print debugging information" },
  { "write-lp", ITC2007_OPT_WRITE_LP, "FILE", 0, "Write .lp to file (only for exact solver)" },
  { "draw-all", ITC2007_OPT_DRAW_DIRECTORY, "DIR", 0,
        "Draw all the timetables of the solution and place them in DIR" },
  { "draw-overview", ITC2007_OPT_DRAW_OVERVIEW_FILE, "FILE", 0,
        "Draw the overview timetable of the solution and save it as FILE" },
  { "force-draw", ITC2007_OPT_FORCE_DRAW, NULL, 0,
        "Draw even if a feasible solution can't be provided" },
  { "time", ITC2007_OPT_TIME_LIMIT, "SECONDS", 0, "Time limit in seconds for solve the model" },
  { "solution", ITC2007_OPT_SOLUTION, "SOL_FILE", 0, "Load the solution file SOL_FILE instead of computing it "
                                                     "(useful for see the cost/violations or with -d or -D for render the solution)" },
  { "method", ITC2007_OPT_METHOD, "exact|tabu", 0, "Method to use for solve the model.\n"
                                                    "Possible values are: 'exact'" },
  { "seed", ITC2007_OPT_SEED, "N", 0, "Seed to use for randomization, if not provided a random one is used" },
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

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch (key) {
    case ITC2007_OPT_VERBOSE:
        args->verbose = true;
        break;
    case ITC2007_OPT_DRAW_DIRECTORY:
        args->draw_directory = arg;
        break;
    case ITC2007_OPT_DRAW_OVERVIEW_FILE:
        args->draw_overview_file = arg;
        break;
    case ITC2007_OPT_FORCE_DRAW:
        args->force_draw = true;
        break;
    case ITC2007_OPT_SOLUTION:
        args->solution_input_file = arg;
        break;
    case ITC2007_OPT_WRITE_LP:
        args->write_lp_file = arg;
        break;
    case ITC2007_OPT_TIME_LIMIT:
        args->time_limit = parse_int(arg);
        break;
    case ITC2007_OPT_METHOD:
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
    case ITC2007_OPT_SEED:
        args->seed = parse_uint(arg);
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