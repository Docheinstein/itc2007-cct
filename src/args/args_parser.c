#include "args_parser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include "utils/str_utils.h"
#include "utils/io_utils.h"
#include "log/debug.h"

const char *argp_program_version = "0.1";
static const char *argp_args_doc = "MODEL [OUTPUT]";
static const char *argp_doc =
    // PRE_DOC
    "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007"
    "\v"
    // POST_DOC
    "The solver supports the following heuristics methods:\n"
    "Local Search, Hill Climbing, Tabu Search and Simulated Annealing.\n"
    "\n"
    "The solver itself and each method can be customized via a config file (-c FILE) "
    "or giving options at the command line (-o KEY=VALUE).\n"
    "\n"
    "The following options are currently supported:\n"
    "\n"
    "# Comma separated list of methods among 'ls', 'hc', 'ts', 'sa'.\n"
    "# Default: hc,sa\n"
    "solver.methods=hc,sa\n"
    "\n"
    "# Solve for no more than N seconds.\n"
    "# Default: 60\n"
    "solver.max_time=60\n"
    "\n"
    "# Solve for no more than N cycles.\n"
    "# Default: -1\n"
    "solver.max_cycles=-1\n"
    "\n"
    "# Whether generate a new initial solution each cycle.\n"
    "# Default: false\n"
    "solver.multistart=false\n"
    "\n"
    "# Restore the best solution so far after N non improving cycles.\n"
    "# (does nothing if multistart is true).\n"
    "# Default: 20\n"
    "solver.restore_best_after_cycles=20\n"
    "\n"
    "# Randomness of the initial feasible solution.\n"
    "# (0 produces always the same deterministic solution,\n"
    "# higher value produces \"more random\" solutions,\n"
    "# but makes it harder to find feasible ones).\n"
    "# Default: 0.33\n"
    "finder.ranking_randomness=0.33\n"
    "\n"
    "# Maximum non-improving iterations number.\n"
    "# Default: 150000\n"
    "hc.max_idle=150000\n"
    "\n"
    "# Maximum non-improving iterations number.\n"
    "# Default: 4000\n"
    "ts.max_idle=4000\n"
    "\n"
    "# Tabu tenure (number of iterations a move remain banned).\n"
    "# Default: 80\n"
    "ts.tabu_tenure=80\n"
    "\n"
    "# Coefficient of penalty for frequently banned moves.\n"
    "# tt(m) = tt + ts.frequency_penalty_coeff * freq(m)\n"
    "# Default: 0\n"
    "ts.frequency_penalty_coeff=0\n"
    "\n"
    "# Maximum non-improving iterations number.\n"
    "# Default: -1\n"
    "sa.max_idle=-1\n"
    "\n"
    "# Initial temperature.\n"
    "# Default: 1.5\n"
    "sa.initial_temperature=1.5\n"
    "\n"
    "# Temperature's cooling rate.\n"
    "# Default: 0.995\n"
    "sa.cooling_rate=0.995\n"
    "\n"
    "# Minimum temperature to reach before leave the method.\n"
    "# Default: 0.10\n"
    "sa.min_temperature=0.10\n"
    "\n"
    "# Coefficient for the number of iterations to do\n"
    "# using the same temperature.\n"
    "# temperature_length = sa.temperature_length_coeff * n_lectures\n"
    "# Default: 1\n"
    "sa.temperature_length_coeff=1"
;

typedef enum itc2007_option {
    OPTION_VERBOSE = 'v',
    OPTION_SEED = 's',
    OPTION_MAX_TIME = 't',
    OPTION_CONFIG = 'c',
    OPTION_OPTION = 'o',
    OPTION_BENCHMARK_MODE = 'b',
    OPTION_DONT_SOLVE = '0',
    OPTION_INPUT_SOLUTION = 'i',
    OPTION_DRAW_ALL_DIRECTORY = 'D',
    OPTION_DRAW_OVERVIEW_FILE = 'd'
} itc2007_option;

static struct argp_option options[] = {
  { "verbose", OPTION_VERBOSE, NULL, 0,
        "Print more information (-vv for print even more)" },
  { "seed", OPTION_SEED, "N", 0,
        "Seed to use for random number generator" },
  { "time", OPTION_MAX_TIME, "SECONDS", 0,
        "Time limit in seconds (override solver.max_time)" },
  { "config", OPTION_CONFIG, "FILE", 0,
        "Load config from FILE" },
  { "option", OPTION_OPTION, "KEY=VALUE", 0,
        "Set an option KEY to VALUE (as if it was specified in a config file)" },
  { "benchmark", OPTION_BENCHMARK_MODE, NULL, 0,
        "Benchmark mode: append a line of stats to OUTPUT (instead of writing the solution) with the format <seed> <fingerprint> <feasible> <rc> <mwd> <cc> <rs> <cost>" },
  { "no-solve", OPTION_DONT_SOLVE, NULL, 0,
        "Do not solve the model: can be useful with -i to print the cost of a loaded solution, and with -d to it render it" },
  { "draw-all", OPTION_DRAW_ALL_DIRECTORY, "DIR", 0,
        "Draw all the timetables (room, teacher, course, curricula) of the solution and place them in DIR" },
  { "draw-overview", OPTION_DRAW_OVERVIEW_FILE, "FILE", 0,
        "Draw only the overview timetable of the solution and save it as FILE" },
  { "solution", OPTION_INPUT_SOLUTION, "FILE", 0,
        "Load the initial solution from FILE instead of find it.\n"
        "Can be useful to continue the computation from a certain solution, eventually using another method.\n"
        "Can be also useful with -0 and/or -d to print the cost of a previously computed solution and/or to render its timetable."
        "For print only the cost of a solution (as a validator) without use the solver, use -0i FILE." },
  { NULL }
};

typedef struct argp_input_arg {
    args_parser *parser;
    args *args;
} argp_input_arg;

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct argp_input_arg *in = state->input;
    args *args = in->args;
    args_parser *parser = in->parser;

#define PARSE_X(converter, str, var, errmsg, ...) do { \
    if (!converter(str, var)) { \
        parser->error = strmake(errmsg, #__VA_ARGS__); \
        return EINVAL; \
    } \
} while(0)

#define PARSE_INT(str, var) PARSE_X(strtoint, str, var, "integer conversion failed ('%s')", str)
#define PARSE_UINT(str, var) PARSE_X(strtouint, str, var, "integer conversion failed ('%s')", str)
#define PARSE_DOUBLE(str, var) PARSE_X(strtodouble, str, var, "double conversion failed ('%s')", str)

#ifdef DEBUG1
    if (isascii(key))
        debug("Parsing argument %s%c%s%s",
              key ? "-" : "", key ? key : ' ' , arg ? " " : "", arg ? arg : "");
#endif

    switch (key) {
    case OPTION_VERBOSE:
        args->verbosity++;
        break;
    case OPTION_SEED:
        PARSE_UINT(arg, &args->seed);
        break;
    case OPTION_MAX_TIME:
        PARSE_INT(arg, &args->max_time);
        break;
    case OPTION_CONFIG:
        args->config_file = arg;
        break;
    case OPTION_OPTION:
        g_array_append_val(args->options, arg);
        break;
    case OPTION_BENCHMARK_MODE:
        args->benchmark_mode = true;
        break;
    case OPTION_DONT_SOLVE:
        args->dont_solve = true;
        break;
    case OPTION_DRAW_ALL_DIRECTORY:
        args->draw_all_directory = arg;
        break;
    case OPTION_DRAW_OVERVIEW_FILE:
        args->draw_overview_file = arg;
        break;
    case OPTION_INPUT_SOLUTION:
        args->solution_input_file = arg;
        break;
    case ARGP_KEY_ARG:
        switch (state->arg_num) {
        case 0:
            args->input_file = arg;
            break;
        case 1:
            args->output_file = arg;
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

#undef PARSE_X
#undef PARSE_INT
#undef PARSE_UINT
#undef PARSE_DOUBLE

    return 0;
}

bool parse_args(args *args, int argc, char **argv) {
    args_parser parser;
    args_parser_init(&parser);

    bool success = args_parser_parse(&parser, argc, argv, args);
    if (!success)
        eprint("ERROR: failed to parse args (%s)", args_parser_get_error(&parser));

    args_parser_destroy(&parser);

    return success;
}

void args_parser_init(args_parser *parser) {
    parser->error = NULL;
}

void args_parser_destroy(args_parser *parser) {
    free(parser->error);
}

bool args_parser_parse(args_parser *parser, int argc, char **argv, args *args) {
    argp_input_arg in = {
        .args = args,
        .parser = parser
    };

    struct argp argp = {options, parse_option, argp_args_doc, argp_doc};
    argp_parse(&argp, argc, argv, 0, 0, &in);

    return strempty(parser->error);
}

char *args_parser_get_error(args_parser *parser) {
    return parser->error;
}
