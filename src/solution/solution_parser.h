#ifndef SOLUTION_PARSER_H
#define SOLUTION_PARSER_H

#include <stdbool.h>
#include "solution.h"

/*
 * Parser of ITC2007 solutions.
 *
 * This is not strictly necessary for solve the model,
 * but is useful for debug solutions, start the resolution
 * from a previously interrupted solution or to render
 * timetables of solutions.
 */

typedef struct solution_parser {
    char *error;
    void *_state;
} solution_parser;

bool parse_solution(solution *sol, const char *filename);

void solution_parser_init(solution_parser *parser);
void solution_parser_destroy(solution_parser *parser);
bool solution_parser_parse(solution_parser *parser, const char *filename,
                           solution *solution);
const char * solution_parser_get_error(solution_parser *parser);


#endif // SOLUTION_PARSER_H