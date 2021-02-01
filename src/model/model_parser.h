#ifndef MODEL_PARSER_H
#define MODEL_PARSER_H

#include "model.h"
#include <stdbool.h>

/*
 * Parser of ITC2007 instances.
 */

typedef struct model_parser {
    char *error;
    void *_state;
} model_parser;

bool parse_model(model *model, const char *filename);

void model_parser_init(model_parser *parser);
void model_parser_destroy(model_parser *parser);

bool model_parser_parse(model_parser *parser, const char *filename, model *model);

const char *model_parser_get_error(model_parser *parser);


#endif // MODEL_PARSER_H