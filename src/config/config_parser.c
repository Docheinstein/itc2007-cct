#include "config_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <string.h>
#include <utils/mem_utils.h>
#include "utils/io_utils.h"

void config_parser_init(config_parser *parser) {
    parser->error = NULL;
}

void config_parser_destroy(config_parser *parser) {
    free(parser->error);
}

static bool config_parser_handle_key_value(config *cfg, char *key, char *value,
                                           char *error_reason, int error_reason_len) {

#define PARSE_INT(var, str) do { \
    bool ok; \
    (var) = strtoint(str, &ok); \
    if (!ok) { \
        snprintf(error_reason, error_reason_len, \
                "integer conversion failed ('%s')", str);\
        return false; \
    }  \
} while(0)

#define PARSE_DOUBLE(var, str) do { \
    bool ok; \
    (var) = strtodouble(str, &ok); \
    if (!ok) { \
        snprintf(error_reason, error_reason_len, \
                "double conversion failed ('%s')", str);\
        return false; \
    }  \
} while(0)

#define PARSE_BOOL(var, str) do { \
    if (streq(str, "true") || streq(str, "yes") || streq(str, "1")) \
        (var) = true;  \
    else if(streq(str, "false") || streq(str, "no") || streq(str, "0")) \
        (var) = false; \
    else { \
        snprintf(error_reason, error_reason_len, \
                "boolean conversion failed ('%s')", str);\
        return false; \
    }  \
} while(0)

    debug("config_parser_handle_key_value (%s,%s)", key, value);

    if (streq(key, "solver.cycles")) {
        PARSE_INT(cfg->solver_cycles_limit, value);
    }
    else if (streq(key, "solver.time")) {
        PARSE_INT(cfg->solver_time_limit, value);
    }
    else if (streq(key, "solver.multistart")) {
        PARSE_BOOL(cfg->solver_multistart, value);
    }
    else if (streq(key, "solver.methods")) {
        static const int MAX_METHODS = 10;
        if (!cfg->solver_methods)
            cfg->solver_methods = mallocx(MAX_METHODS, sizeof(resolution_method));
        char **methods_strings = mallocx(MAX_METHODS, sizeof(char *));
        cfg->solver_n_methods = strsplit(value, ",", methods_strings, MAX_METHODS);

        for (int i = 0; i < cfg->solver_n_methods; i++) {
            char *method = strtrim(methods_strings[i]);
            debug("Found method: '%s'", method);
            if (streq(method, "ls"))
                cfg->solver_methods[i] = RESOLUTION_METHOD_LOCAL_SEARCH;
            else if (streq(method, "ts"))
                cfg->solver_methods[i] = RESOLUTION_METHOD_TABU_SEARCH;
            else if (streq(method, "hc"))
                cfg->solver_methods[i] = RESOLUTION_METHOD_HILL_CLIMBING;
            else if (streq(method, "sa"))
                cfg->solver_methods[i] = RESOLUTION_METHOD_SIMULATED_ANNEALING;
            else {
                print("WARN: unexpected method, skipping '%s' (possible values are 'ls', 'ts', 'hc', 'sa'", method);
                cfg->solver_methods[i] = RESOLUTION_METHOD_NONE;
            }
        }
        free(methods_strings);
    }
    else if (streq(key, "hc.idle")) {
        PARSE_INT(cfg->hc_idle, value);
    }
    else if (streq(key, "ts.idle")) {
        PARSE_INT(cfg->ts_idle, value);
    }
    else if (streq(key, "ts.tenure")) {
        PARSE_INT(cfg->ts_tenure, value);
    }
    else if (streq(key, "ts.frequency_penalty_coeff")) {
        PARSE_DOUBLE(cfg->ts_frequency_penalty_coeff, value);
    }
    else if (streq(key, "ts.random_pick")) {
        PARSE_BOOL(cfg->ts_random_pick, value);
    }
    else if (streq(key, "ts.steepest")) {
        PARSE_BOOL(cfg->ts_steepest, value);
    }
    else if (streq(key, "ts.clear_on_new_best")) {
        PARSE_BOOL(cfg->ts_clear_on_new_best, value);
    }
    else {
        print("WARN: unexpected key, skipping '%s'", key);
    }


#undef PARSE_INT
#undef PARSE_BOOL
#undef PARSE_DOUBLE

    return true;
};



static bool config_parser_line_parser(
        const char *line, void *arg,
        char *error_reason, int error_reason_len) {
    config *cfg = (config *) arg;
    char *tokens[2];

    char *line_copy = strdup(line);

    if (strsplit(line_copy, "=", tokens, 2) != 2) {
        print("WARN: invalid line syntax, skipping '%s'", line);
        return true;
    }

    char *key = strtrim(tokens[0]);
    char *value = strtrim_chars(tokens[1], "\" ");

    bool continue_parse = config_parser_handle_key_value(
            cfg, key, value, error_reason, error_reason_len);

    free(line_copy);

    return continue_parse;
}


bool config_parser_parse_option(config_parser *parser, config *config, const char *option) {
    char error_reason[256];
    error_reason[0] = '\0';
    config_parser_line_parser(option, config, error_reason, 256);
    if (!strempty(error_reason)) {
        snprintf(parser->error, 256, "%s", error_reason);
        return false;
    }
    return true;
}

bool config_parser_parse_file(config_parser *parser, config *config, const char *input_file) {
    fileparse_options options = { .comment_prefix = '#' };
    parser->error = fileparse(input_file, config_parser_line_parser, config, &options);
    return strempty(parser->error);
}

const char *config_parser_get_error(config_parser *parser) {
    return parser->error;
}

bool read_config_file(config *config, const char *input_file) {
    config_parser parser;
    config_parser_init(&parser);
    bool success = config_parser_parse_file(&parser, config, input_file);
    if (!success) {
        eprint("ERROR: failed to parse config file '%s' (%s)", input_file, config_parser_get_error(&parser));
    }

    config_parser_destroy(&parser);

    return success;
}

bool read_config_options(config *config, const char **options, int n_options) {
    config_parser parser;
    config_parser_init(&parser);
    bool success = true;
    int i;
    for (i = 0; i < n_options && success; i++)
        success = config_parser_parse_option(&parser, config, options[i]);

    if (!success) {
        eprint("ERROR: failed to parse option '%s' (%s)", options[i - 1], config_parser_get_error(&parser));
    }

    config_parser_destroy(&parser);

    return success;
}
