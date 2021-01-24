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

    debug("config_parser_handle_key_value (%s,%s)", key, value);

    if (streq(key, "hc.idle")) {
        PARSE_INT(cfg->hc_idle, value);
    }
    else if (streq(key, "solver.methods")) {
        static const int MAX_METHODS = 10;
        if (!cfg->methods)
            cfg->methods = mallocx(MAX_METHODS, sizeof(resolution_method));
        char **methods_strings = mallocx(MAX_METHODS, sizeof(char *));
        cfg->n_methods = strsplit(value, ",", methods_strings, MAX_METHODS);

        for (int i = 0; i < cfg->n_methods; i++) {
            char *method = strtrim(methods_strings[i]);
            debug("Found method: '%s'", method);
            if (streq(method, "ls"))
                cfg->methods[i] = RESOLUTION_METHOD_ITERATED_LOCAL_SEARCH;
            else if (streq(method, "ts"))
                cfg->methods[i] = RESOLUTION_METHOD_TABU_SEARCH;
            else if (streq(method, "hc"))
                cfg->methods[i] = RESOLUTION_METHOD_HILL_CLIMBING;
            else if (streq(method, "sa"))
                cfg->methods[i] = RESOLUTION_METHOD_SIMULATED_ANNEALING;
            else {
                print("WARN: unexpected method, skipping '%s' (possible values are 'ls', 'ts', 'hc', 'sa'", method);
                cfg->methods[i] = RESOLUTION_METHOD_NONE;
            }
        }
        free(methods_strings);
    }
    else {
        print("WARN: unexpected key, skipping '%s'", key);
    }


#undef PARSE_INT

    return true;
};

static bool config_parser_callback(
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

bool config_parser_parse(config_parser *parser, const char *input_file, config *cfg) {
    fileparse_options options = { .comment_prefix = '#' };
    parser->error = fileparse(input_file, config_parser_callback, cfg, &options);
    return strempty(parser->error);
}

const char *config_parser_get_error(config_parser *parser) {
    return parser->error;
}
