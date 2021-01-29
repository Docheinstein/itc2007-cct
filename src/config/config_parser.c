#include "config_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/mem_utils.h>
#include <string.h>
#include "utils/io_utils.h"
#include "config.h"

static const int MAX_METHODS = 10;

bool parse_config_file(config *config, const char *filename) {
    config_parser parser;
    config_parser_init(&parser);
    bool success = config_parser_parse(&parser, config, filename);
    if (!success) {
        eprint("ERROR: failed to parse config file '%s' (%s)",
               filename, config_parser_get_error(&parser));
    }

    config_parser_destroy(&parser);

    return success;
}

bool parse_config_options(config *config, const char **options, int n_options) {
    config_parser parser;
    config_parser_init(&parser);
    bool success = true;
    int i;
    for (i = 0; i < n_options && success; i++)
        success = config_parser_add_option(&parser, config, options[i]);

    if (!success)
        eprint("ERROR: failed to parse option '%s' (%s)",
               options[i - 1], config_parser_get_error(&parser));

    config_parser_destroy(&parser);

    return success;
}



void config_parser_init(config_parser *parser) {
    parser->error = NULL;
}

void config_parser_destroy(config_parser *parser) {
    free(parser->error);
}

static void config_parser_add_method(config *cfg, const char *method) {
    debug("Adding method: '%s'", method);
    heuristic_method m;

    if (streq(method, "ls"))
        m = HEURISTIC_METHOD_LOCAL_SEARCH;
    else if (streq(method, "ts"))
        m = HEURISTIC_METHOD_TABU_SEARCH;
    else if (streq(method, "hc"))
        m = HEURISTIC_METHOD_HILL_CLIMBING;
    else if (streq(method, "sa"))
        m = HEURISTIC_METHOD_SIMULATED_ANNEALING;
    else {
        print("WARN: unexpected method, skipping '%s' (possible values are 'ls', 'ts', 'hc', 'sa'", method);
        return;
    }

    g_array_append_val(cfg->solver.methods, m);
}

static char * config_parser_key_value_handler(config *cfg, char *key, char *value) {

#define PARSE_INT(str, var) strtoint(str, var) ? NULL: strmake("integer conversion failed ('%s')", str)
#define PARSE_DOUBLE(str, var) strtodouble(str, var) ? NULL: strmake("double conversion failed ('%s')", str)
#define PARSE_BOOL(str, var) strtobool(str, var) ? NULL: strmake("boolean conversion failed ('%s')", str)

    debug("Parsing config line: %s=%s", key, value);

    if (streq(key, "solver.methods")) {
        g_array_remove_range(cfg->solver.methods, 0, cfg->solver.methods->len);
        char **methods_strings = mallocx(MAX_METHODS, sizeof(char *));
        int n_methods = strsplit(value, ",", methods_strings, MAX_METHODS);
        for (int i = 0; i < n_methods; i++)
            config_parser_add_method(cfg, strtrim(methods_strings[i]));
        free(methods_strings);
        return NULL;
    }

    if (streq(key, "solver.max_time"))
        return PARSE_INT(value, &cfg->solver.max_time);
    if (streq(key, "solver.max_cycles"))
        return PARSE_INT(value, &cfg->solver.max_cycles);
    if (streq(key, "solver.multistart"))
        return PARSE_BOOL(value, &cfg->solver.multistart);
    if (streq(key, "solver.restore_best_after_cycles"))
        return PARSE_INT(value, &cfg->solver.restore_best_after_cycles);

    if (streq(key, "finder.ranking_randomness"))
        return PARSE_DOUBLE(value, &cfg->finder.ranking_randomness);

    if (streq(key, "ls.steepest"))
        return PARSE_BOOL(value, &cfg->ls.steepest);

    if (streq(key, "hc.max_idle"))
        return PARSE_INT(value, &cfg->hc.max_idle);

    if (streq(key, "ts.max_idle"))
        return PARSE_INT(value, &cfg->ts.max_idle);
    if (streq(key, "ts.tabu_tenure"))
        return PARSE_INT(value, &cfg->ts.tabu_tenure);
    if (streq(key, "ts.frequency_penalty_coeff"))
        return PARSE_DOUBLE(value, &cfg->ts.frequency_penalty_coeff);
    if (streq(key, "ts.random_pick"))
        return PARSE_BOOL(value, &cfg->ts.random_pick);
    if (streq(key, "ts.steepest"))
        return PARSE_BOOL(value, &cfg->ts.steepest);
    if (streq(key, "ts.clear_on_best"))
        return PARSE_BOOL(value, &cfg->ts.clear_on_best);

    if (streq(key, "sa.max_idle"))
        return PARSE_INT(value, &cfg->sa.max_idle);
    if (streq(key, "sa.initial_temperature"))
        return PARSE_DOUBLE(value, &cfg->sa.initial_temperature);
    if (streq(key, "sa.cooling_rate"))
        return PARSE_DOUBLE(value, &cfg->sa.cooling_rate);
    if (streq(key, "sa.min_temperature"))
        return PARSE_DOUBLE(value, &cfg->sa.min_temperature);
    if (streq(key, "sa.temperature_length_coeff"))
        return PARSE_DOUBLE(value, &cfg->sa.temperature_length_coeff);

    print("WARN: unexpected key, skipping '%s'", key);

#undef PARSE_INT
#undef PARSE_BOOL
#undef PARSE_DOUBLE

    return NULL;
};


static char * config_parser_line_handler(const char *line, void *arg) {
    config *cfg = (config *) arg;

    char *fields[2];
    char *error = NULL;
    char *line_copy = strdup(line);

    if (strsplit(line_copy, "=", fields, 2) != 2) {
        print("WARN: invalid line syntax, skipping '%s'", line);
        goto QUIT;
    }

    char *key = strtrim(fields[0]);
    char *value = strtrim_chars(fields[1], "\" ");

    error = config_parser_key_value_handler(cfg, key, value);

QUIT:
    free(line_copy);

    return error;
}


bool config_parser_add_option(config_parser *parser, config *config, const char *option) {
    debug("Adding option to config: %s", option);
    parser->error = config_parser_line_handler(option, config);
    return strempty(parser->error);
}


bool config_parser_parse(config_parser *parser, config *config, const char *filename) {
    fileparse_options options = { .comment_prefix = '#' };
    parser->error = fileparse(filename,  &options, config_parser_line_handler, config);
    return strempty(parser->error);
}

const char *config_parser_get_error(config_parser *parser) {
    return parser->error;
}