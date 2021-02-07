#include "args_parser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include "utils/str_utils.h"
#include "utils/io_utils.h"
#include "log/debug.h"

const char *argp_program_version = "1.0";
static const char *argp_args_doc = "INPUT [OUTPUT]";
static const char *argp_doc =
    // PRE_DOC
    "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007"
    "\v"
    // POST_DOC
    "The solver supports the following heuristics methods:\n"
    "Local Search, Hill Climbing, Tabu Search, Simulated Annealing and Deep Local Search.\n"
    "\n"
    "The solver itself and each heuristics can be customized via a config file (-c FILE) "
    "or giving options at the command line (-o KEY=VALUE).\n"
    "\n"
    "The following options are currently supported:\n"
    "\n"
    "SOLVER\n"
    "\n"
    "# Comma separated list of methods among 'ls', 'hc', 'ts', 'sa', 'dls'.\n"
    "solver.methods=sa,ls\n"
    "\n"
    "# Solve for no more than N seconds.\n"
    "solver.max_time=60\n"
    "\n"
    "# Solve for no more than N cycles.\n"
    "solver.max_cycles=-1\n"
    "\n"
    "# Whether generate a new initial solution each cycle.\n"
    "solver.multistart=false\n"
    "\n"
    "# Restore the best solution so far after N non improving cycles.\n"
    "# (does nothing if multistart is true).\n"
    "solver.restore_best_after_cycles=50\n"
    "\n"
    "FINDER\n"
    "\n"
    "# Randomness of the initial feasible solution.\n"
    "# (0 produces always the same deterministic solution,\n"
    "# an higher value produces \"more random\" solutions,\n"
    "# but makes it harder to find feasible ones).\n"
    "finder.ranking_randomness=0.33\n"
    "\n"
    "SIMULATED ANNEALING\n"
    "\n"
    "# Initial temperature.\n"
    "sa.initial_temperature=1.4\n"
    "\n"
    "# Temperature's cooling rate.\n"
    "sa.cooling_rate=0.965\n"
    "\n"
    "# Minimum temperature to reach before leave the method.\n"
    "sa.min_temperature=0.12\n"
    "\n"
    "# Decrease the minimum temperature to reach to \n"
    "# sa.min_temperature * sa.min_temperature_near_best_coeff if near\n"
    "# the best; the meaning of \"near\" can be customized with sa.near_best_ratio.\n"
    "sa.min_temperature_near_best_coeff=0.68\n"
    "\n"
    "# Consider a solution \"near best\" if the current solution has cost\n"
    "# equal or less sa.near_best_ratio times the best solution cost.\n"
    "sa.near_best_ratio=1.05\n"
    "\n"
    "# Coefficient for the number of iterations with the same temperature.\n"
    "# temperature_length = sa.temperature_length_coeff * L * R* * D * S\n"
    "sa.temperature_length_coeff=0.125\n"
    "\n"
    "LOCAL SEARCH\n"
    "\n"
    "# Do nothing if the current solution has cost greater than \n"
    "# ls.max_distance_from_best_ratio times the best solution cost.\n"
    "ls.max_distance_from_best_ratio=-1\n"
    "\n"
    "HILL CLIMBING\n"
    "\n"
    "# Maximum non-improving iterations number.\n"
    "hc.max_idle=120000\n"
    "\n"
    "# Increase hc.max_idle to hc.max_idle * by hc.max_idle_near_best_coeff\n"
    "# if near the best (intensification); the meaning of \"near\" can be\n"
    "# customized with hc.near_best_ratio.\n"
    "hc.max_idle_near_best_coeff=3\n"
    "\n"
    "# Consider a solution \"near best\" if the current solution has cost\n"
    "# equal or less hc.near_best_ratio times the best solution cost.\n"
    "hc.near_best_ratio=1.02\n"
    "\n"
    "TABU SEARCH\n"
    "\n"
    "# Maximum non-improving iterations number.\n"
    "ts.max_idle=-1\n"
    "\n"
    "# Tabu tenure (number of iterations a move remain banned).\n"
    "ts.tabu_tenure=120\n"
    "\n"
    "# Coefficient of penalty for frequently banned moves.\n"
    "# tt(m) = tt + ts.frequency_penalty_coeff * freq(m)\n"
    "ts.frequency_penalty_coeff=0\n"
    "\n"
    "# Increase ts.max_idle to ts.max_idle * by ts.max_idle_near_best_coeff\n"
    "# if near the best (intensification); the meaning of \"near\" can be\n"
    "# customized with hc.near_best_ratio.\n"
    "ts.max_idle_near_best_coeff=1.5\n"
    "\n"
    "# Consider a solution \"near best\" if the current solution has cost\n"
    "# equal or less ts.near_best_ratio times the best solution cost.\n"
    "ts.near_best_ratio=1.02\n"
    "\n"
    "DEEP LOCAL SARCH\n"
    "\n"
    "# Do nothing if the current solution has cost greater than\n"
    "# dls.max_distance_from_best_ratio times the best solution cost.\n"
    "dls.max_distance_from_best_ratio=-1"
;

typedef enum itc2007_option {
    OPTION_VERBOSE = 'v',
    OPTION_SEED = 's',
    OPTION_MAX_TIME = 't',
    OPTION_CONFIG = 'c',
    OPTION_OPTION = 'o',
    OPTION_BENCHMARK_FILE = 'b',
    OPTION_RACE_MODE = 'r',
    OPTION_QUIET = 'q',
    OPTION_DONT_SOLVE = '0',
    OPTION_PRINT_VIOLATIONS = 'V',
    OPTION_INPUT_SOLUTION = 'i',
    OPTION_INPUT_SOLUTION_VALIDATE = 'I',
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
  { "benchmark", OPTION_BENCHMARK_FILE, "FILE", OPTION_ARG_OPTIONAL,
        "Benchmark mode: print a line of stats with the format "
        "<seed> <fingerprint> <cycles> <moves> <feasible> <rc> <mwd> <cc> <rs> <cost> "
        "and append it to FILE if it is given.\n"
        "When used with -r, append a line of stats each time a new best solution is found." },
  { "race", OPTION_RACE_MODE, NULL, 0,
        "Race mode: run forever (unless -t is explicitly given).\n"
        "When a new best solution is found, it is handled immediately "
        "(and depending on other options it is printed to stdout, written to OUTPUT, drawn, etc.)." },
  { "quiet", OPTION_QUIET, NULL, 0,
        "Does not print the solution to stdout" },
  { "no-solve", OPTION_DONT_SOLVE, NULL, 0,
        "Do not solve the model: can be used with -i to load a solution without solving it" },
  { "print-violations", OPTION_PRINT_VIOLATIONS, NULL, 0,
        "Print the costs and the violations of the solution (-VV for print the details of each violation)"},
  { "draw-all", OPTION_DRAW_ALL_DIRECTORY, "DIR", 0,
        "Draw all the timetables (room, teacher, course, curricula) of the solution "
        "and place them in DIR" },
  { "draw-overview", OPTION_DRAW_OVERVIEW_FILE, "FILE", 0,
        "Draw only the overview timetable of the solution and save it as FILE" },
  { "solution", OPTION_INPUT_SOLUTION, "FILE", 0,
        "Load the initial solution from FILE instead of find it. "
        "Can be useful to continue the computation from a previously computed solution (eventually using another method). "
        "For print only the costs and the violations of a solution (as a validator) "
        "without use the solver, use -0VVqi FILE."},
  { "validate", OPTION_INPUT_SOLUTION_VALIDATE, "FILE", 0,
        "Print only the costs and the violations of a solution in FILE (as a validator).\n"
        "Alias for -0VVqi FILE."},
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
        parser->error = strmake(errmsg, ##__VA_ARGS__); \
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
    case OPTION_BENCHMARK_FILE:
        args->benchmark_mode = true;
        args->benchmark_file = arg;
        break;
    case OPTION_RACE_MODE:
        args->race_mode = true;
        break;
    case OPTION_QUIET:
        args->quiet = true;
        break;
    case OPTION_DONT_SOLVE:
        args->dont_solve = true;
        break;
    case OPTION_PRINT_VIOLATIONS:
        args->print_violations++;
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
    case OPTION_INPUT_SOLUTION_VALIDATE:
        // Alias for -0VVqi
        parse_option(OPTION_DONT_SOLVE, NULL, state);
        parse_option(OPTION_PRINT_VIOLATIONS, NULL, state);
        parse_option(OPTION_PRINT_VIOLATIONS, NULL, state);
        parse_option(OPTION_QUIET, NULL, state);
        parse_option(OPTION_INPUT_SOLUTION, arg, state);
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
