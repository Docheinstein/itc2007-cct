#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdbool.h>
#include "config.h"

typedef struct config_parser {
    char *error;
} config_parser;

bool read_config_file(config *config, const char *input_file);
bool read_config_options(config *config, const char **options, int n_options);

void config_parser_init(config_parser *parser);
void config_parser_destroy(config_parser *parser);

bool config_parser_parse_file(config_parser *parser, config *config,
                              const char *input_file);

bool config_parser_parse_option(config_parser *parser, config *config,
                                const char *option);

const char *config_parser_get_error(config_parser *parser);


#endif // CONFIG_PARSER_H