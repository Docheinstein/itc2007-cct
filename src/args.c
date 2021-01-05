#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include "args.h"
#include "str_utils.h"
#include "io_utils.h"
#include "def_utils.h"
#include <stdbool.h>

static const char * itc2007_method_to_string(itc2007_method method) {
    switch (method) {
    case ITC2007_METHOD_EXACT:
        return "exact";
    case ITC2007_METHOD_TABU:
        return "tabu";
    default:
        return "unknown";
    }
}
void args_to_string(const args *args, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
        "input = %s\n"
        "output = %s\n"
        "verbose = %s\n"
        "method = %s\n"
        "write_lp = %s\n"
        "time_limit = %d",
        args->input,
        args->output,
        BOOL_TO_STR(args->verbose),
        itc2007_method_to_string(args->method),
        args->write_lp_file,
        args->time_limit
    );
}

void args_init(args *args) {
    args->input = NULL;
    args->output = NULL;
    args->verbose = false;
    args->method = ITC2007_METHOD_DEFAULT;
    args->draw_dir = NULL;
    args->write_lp_file = NULL;
    args->time_limit = 0;
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
    ITC2007_OPT_DRAW = 'd',
    ITC2007_OPT_WRITE_LP = 0x100,
} itc2007_option;


static struct argp_option options[] = {
  { "verbose", ITC2007_OPT_VERBOSE, NULL, 0, "Print debugging information" },
  { "write-lp", ITC2007_OPT_WRITE_LP, "FILE", 0, "Write .lp to file (only for exact solver)" },
  { "draw", ITC2007_OPT_DRAW, "DIR", 0, "Draw graphical representations of the solution "
                                        "and place them in DIR" },
  { "time", ITC2007_OPT_TIME_LIMIT, "SECONDS", 0, "Time limit in seconds for solve the model" },
  { "method", ITC2007_OPT_METHOD, "exact|tabu", 0, "Method to use for solve the model.\n"
                                                    "Possible values are: 'exact'" },
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

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch (key) {
    case ITC2007_OPT_VERBOSE:
        args->verbose = true;
        break;
    case ITC2007_OPT_DRAW:
        args->draw_dir = arg;
        break;
    case ITC2007_OPT_WRITE_LP:
        args->write_lp_file = arg;
        break;
    case ITC2007_OPT_TIME_LIMIT:
        args->time_limit = parse_int(arg);
        break;
    case ITC2007_OPT_METHOD:
        if (streq(arg, "exact"))
            args->method = ITC2007_METHOD_EXACT;
        else if (streq(arg, "tabu"))
            args->method = ITC2007_METHOD_TABU;
        else {
            eprint("ERROR: unknown method '%s'.\n"
                   "Possible values are 'exact', 'tabu'", arg);
            exit(EXIT_FAILURE);
        }
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