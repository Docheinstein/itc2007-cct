#include "solution_parser.h"
#include <stdlib.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include <utils/mem_utils.h>

typedef struct solution_parser_state {
    int *courses_l_cursor;
} solution_parser_state;

static void solution_parser_state_init(solution_parser_state *state, const model *m) {
    MODEL(m);
    state->courses_l_cursor = mallocx(model->n_courses, sizeof(int));
    int courses_l_base_cursor = 0;
    FOR_C {
        state->courses_l_cursor[c] = courses_l_base_cursor;
        courses_l_base_cursor += model->courses[c].n_lectures;
    };
}

static void solution_parser_state_destroy(solution_parser_state *state) {
    free(state->courses_l_cursor);
}

bool parse_solution(solution *sol, const char *filename) {
    if (!filename)
        return false;

    solution_parser parser;
    solution_parser_init(&parser);
    bool success = solution_parser_parse(&parser, filename, sol);
    if (!success)
        eprint("ERROR: failed to parse solution '%s' (%s)",
               filename, solution_parser_get_error(&parser));

    solution_parser_destroy(&parser);

    return success;
}

void solution_parser_init(solution_parser *parser) {
    parser->error = NULL;
    parser->_state = mallocx(1, sizeof(solution_parser_state));
}

void solution_parser_destroy(solution_parser *parser) {
    free(parser->error);
    free(parser->_state);
}


typedef struct solution_parser_line_handler_arg {
    solution_parser *parser;
    solution *solution;
} solution_parser_line_handler_arg;

static char * solution_parser_line_handler(const char *line, void *arg) {
    solution_parser_line_handler_arg *s_arg = (solution_parser_line_handler_arg *) arg;
    solution_parser *parser = s_arg->parser;
    solution_parser_state *state = (solution_parser_state *) parser->_state;
    solution *sol = s_arg->solution;

    MODEL(sol->model);
//    model_parser_state *state = (model_parser_state *) parser->_state;

    char *fields[4];
    char *error = NULL;
    char *line_copy = strdup(line);

    if (strsplit(line_copy, "=", fields, 4) != 4) {
        error = strmake("unexpected fields count: expected 4");
        goto QUIT;
    }

    int c, r, d, s;
    course *course = model_course_by_id(sol->model, fields[0]);
    if (!course) {
        error = strmake("unknown course '%s'", fields[0]);
        goto QUIT;
    }
    c = course->index;

    room *room = model_room_by_id(sol->model, fields[1]);
    if (!room) {
        error = strmake("unknown room '%s'", fields[1]);
        goto QUIT;
    }
    r = room->index;

    if (!strtoint(fields[2], &d)) {
        error = strmake("integer conversion failed ('%s')", fields[2]);
        goto QUIT;
    }

    if (!strtoint(fields[3], &s)) {
        error = strmake("integer conversion failed ('%s')", fields[3]);
        goto QUIT;
    }

    int l = state->courses_l_cursor[c]++;
    if ((c < C - 1 && l >= state->courses_l_cursor[c + 1]) || l > model->n_lectures) {
        error = strmake("unexpected lecture count, unable to parse unfeasible solution", fields[3]);
        goto QUIT;
    }

    solution_set_lecture_assignment(sol, l, r, d, s);

QUIT:
    free(line_copy);

    return error;
}

bool solution_parser_parse(solution_parser *parser,
                           const char *filename, solution *sol) {

    solution_parser_line_handler_arg arg = {
        .parser = parser,
        .solution = sol,
    };

    solution_parser_state_init(parser->_state, sol->model);
    parser->error = fileparse(filename, NULL, solution_parser_line_handler, sol);
    solution_parser_state_destroy(parser->_state);

    return strempty(parser->error);
}

const char *solution_parser_get_error(solution_parser *solution_parser) {
    return solution_parser->error;
}
