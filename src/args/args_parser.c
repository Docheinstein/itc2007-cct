#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include "args_parser.h"
#include "utils/str_utils.h"
#include "utils/io_utils.h"
#include "utils/def_utils.h"
#include <stdbool.h>
#include <log/debug.h>
#include "config.h"


const char *argp_program_version = "0.1";
static const char *doc = "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007";
static const char *args_doc = "INPUT [OUTPUT]";

typedef enum itc2007_option {
    OPTION_VERBOSE = 'v',
    OPTION_METHOD = 'm',
    OPTION_TIME_LIMIT = 't',
    OPTION_THREADS = 'j',
    OPTION_DRAW_DIRECTORY = 'D',
    OPTION_DRAW_OVERVIEW_FILE = 'd',
    OPTION_FORCE_DRAW = 'f',
    OPTION_SOLUTION = 'S',
    OPTION_SEED = 's',
    OPTION_ASSIGNMENTS_DIFFICULTY_RANKING_RANDOMNESS = 'r',
    OPTION_MULTISTART = 'n',
    OPTION_CONFIG = 'c',
    OPTION_OPTION = 'o',
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
  { "threads", OPTION_THREADS, "N", 0,
        "Number of threads to spawn" },
  { "solution", OPTION_SOLUTION, "SOL_FILE", 0,
        "Load the solution file SOL_FILE instead of computing it"
        "(useful for see the cost/violations or with -d or -D for render the solution)" },
  { "method", OPTION_METHOD, "exact|local|tabu", 0,
        "Method to use for solve the model.\n"
        "Possible values are: 'exact', 'local' and 'tabu'" },
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
  { "config", OPTION_CONFIG, "CONF_FILE", 0,
        "Load heuristic solver configuration from file" },
  { "option", OPTION_OPTION, "KEY=VALUE", 0,
        "Set an option" },
  { NULL }
};

static bool parse_int(args_parser *parser, const char *str, int *var) {
    bool ok;
    *var = strtoint(str, &ok);
    if (!ok)
        parser->error = strmake("integer conversion failed for parameter '%s'", str);
    return ok;
}

static bool parse_uint(args_parser *parser, const char *str, uint *var) {
    bool ok;
    *var = strtouint(str, &ok);
    if (!ok) {
        parser->error = strmake("integer conversion failed for parameter '%s'", str);
    }
    return ok;
}

static bool parse_double(args_parser *parser, const char *str, double *var) {
    bool ok;
    *var = strtodouble(str, &ok);
    if (!ok) {
        parser->error = strmake("ERROR: double conversion failed for parameter '%s'", str);
    }
    return ok;
}

typedef struct argp_input_arg {
    args_parser *parser;
    args *args;
} argp_input_arg;

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct argp_input_arg *in = state->input;
    args *args = in->args;
    args_parser *parser = in->parser;

    switch (key) {
    case OPTION_VERBOSE:
        args->verbosity = args->verbosity == ARG_INT_NONE ? 1 : (args->verbosity + 1);
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
        if (!parse_int(parser, arg, &args->time_limit))
            return EINVAL;
        break;
    case OPTION_THREADS:
        if (!parse_int(parser, arg, &args->num_threads))
            return EINVAL;
        break;
    case OPTION_METHOD:
        if (streq(arg, "exact"))
            args->method = RESOLUTION_METHOD_EXACT;
        else if (streq(arg, "local"))
            args->method = RESOLUTION_METHOD_LOCAL_SEARCH;
        else if (streq(arg, "tabu"))
            args->method = RESOLUTION_METHOD_TABU_SEARCH;
        else if (streq(arg, "hill"))
            args->method = RESOLUTION_METHOD_HILL_CLIMBING;
        else if (streq(arg, "iterated"))
            args->method = RESOLUTION_METHOD_ITERATED_LOCAL_SEARCH;
        else if (streq(arg, "annealing"))
            args->method = RESOLUTION_METHOD_SIMULATED_ANNEALING;
        else if (streq(arg, "hybrid"))
            args->method = RESOLUTION_METHOD_HYBRYD;
        else {
            eprint("ERROR: unknown method '%s'.\n"
                   "Possible values are 'exact', 'local', 'tabu', 'hill', 'iterated', 'annealing', 'hybrid'", arg);
            exit(EXIT_FAILURE);
        }
        break;
    case OPTION_SEED:
        if (!parse_uint(parser, arg, &args->seed))
            return EINVAL;
        break;
    case OPTION_ASSIGNMENTS_DIFFICULTY_RANKING_RANDOMNESS:
        if (!parse_double(parser, arg, &args->assignments_difficulty_ranking_randomness))
            return EINVAL;
        break;
    case OPTION_MULTISTART:
        if (!parse_int(parser, arg, &args->multistart))
            return EINVAL;
        break;
    case OPTION_CONFIG:
        args->config = arg;
        break;
    case OPTION_OPTION:
        g_array_append_val(args->options, arg);
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

bool parse_args(args *args, int argc, char **argv) {
    args_parser parser;
    args_parser_init(&parser);

    argp_input_arg in = {
        .args = args,
        .parser = &parser
    };

    struct argp argp = {options, parse_option, args_doc, doc};
    argp_parse(&argp, argc, argv, 0, 0, &in);

    bool success = strempty(parser.error);
    if (!success) {
        eprint("ERROR: failed to parse args (%s)", args_parser_get_error(&parser));
    }

    args_parser_destroy(&parser);

    return success;
}

void args_parser_init(args_parser *parser) {
    parser->error = NULL;
}

void args_parser_destroy(args_parser *parser) {
    free(parser->error);
}

char *args_parser_get_error(args_parser *parser) {
    return parser->error;
}
