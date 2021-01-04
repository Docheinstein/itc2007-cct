#ifndef PARSER_H
#define PARSER_H

#include "model.h"
#include <stdbool.h>

typedef struct parser {
    char *error;
} parser;

void parser_init(parser *parser);
void parser_destroy(parser *parser);

bool parser_parse(parser *parser, char *input, model *model);

const char *parser_get_error(parser *parser);


#endif // PARSER_H