#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include "args.h"

/* Parser of the arguments given at the command line */

typedef struct args_parser {
    char *error;
} args_parser;

bool parse_args(args *args, int argc, char **argv);

void args_parser_init(args_parser *parser);
void args_parser_destroy(args_parser *parser);

bool args_parser_parse(args_parser *parser, int argc, char **argv, args *args);

char *args_parser_get_error(args_parser *parser);

#endif // ARGS_PARSER_H