#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdbool.h>
#include "config.h"

typedef struct config_parser {
    char *error;
} config_parser;

void config_parser_init(config_parser *parser);
void config_parser_destroy(config_parser *parser);

bool config_parser_parse(config_parser *parser, const char *input_file,
                         config *config);

const char *config_parser_get_error(config_parser *parser);


#endif // CONFIG_PARSER_H