#include "model_parser.h"
#include <stdio.h>
#include <string.h>
#include "log/verbose.h"
#include "utils/io_utils.h"
#include "utils/str_utils.h"
#include "utils/mem_utils.h"
#include "model.h"

/*
    INPUT FILE SYNTAX
    =================
    Name: Fis0506-1
    Courses: 30
    Rooms: 6
    Days: 5
    Periods_per_day: 6
    Curricula: 14
    Constraints: 53

    COURSES:
    <course>
    ...

    ROOMS:
    <room>
    ...

    CURRICULA:
    <curricula>
    ...

    UNAVAILABILITY_CONSTRAINTS:
    <unavailability_constraint>
    ...

    END.
    =================

    <course> := <CourseID> <Teacher> <# Lectures> <MinWorkingDays> <# Students>
    <room> := <RoomID> <Capacity>
    <curricula> := <CurriculumID> <# Courses> <CourseID> ... <CourseID>
    <unavailability_constraint> := <CourseID> <Day> <Day_Period>
*/


const char *FIELDS_SEPARATORS = " \t";
const int COURSE_FIELDS = 5;
const int ROOM_FIELDS = 2;
const int CURRICULA_FIXED_FIELDS = 2;
const int UNAVAILABILITY_CONSTRAINTS_FIELDS = 3;

typedef enum model_parser_section {
    MODEL_PARSER_SECTION_NONE,
    MODEL_PARSER_SECTION_COURSES,
    MODEL_PARSER_SECTION_ROOMS,
    MODEL_PARSER_SECTION_CURRICULAS,
    MODEL_PARSER_SECTION_CONSTRAINTS,
} model_parser_section;

typedef struct model_parser_state {
    model_parser_section section;
    int section_cursor;
} model_parser_state;


static void solution_parser_state_init(model_parser_state *state) {
    state->section = MODEL_PARSER_SECTION_NONE;
    state->section_cursor = 0;
}

static void solution_parser_state_destroy(model_parser_state *state) {}

bool parse_model(model *model, const char *filename) {
    model_parser parser;
    model_parser_init(&parser);

    bool success = model_parser_parse(&parser, filename, model);
    if (!success)
        eprint("ERROR: failed to parse model file '%s' (%s)", filename, model_parser_get_error(&parser));

    model_parser_destroy(&parser);

    return success;
}

void model_parser_init(model_parser *parser) {
    parser->error = NULL;
    parser->_state = mallocx(1, sizeof(model_parser_state));
}

void model_parser_destroy(model_parser *parser) {
    free(parser->error);
    free(parser->_state);
}

typedef struct model_parser_line_handler_arg {
    model_parser *parser;
    model *model;
} model_parser_line_handler_arg;

/*
 * `fileparse` callback: real parsing logic.
 */
static char * model_parser_line_handler(const char *line, void *arg) {

#define ABORT_PARSE(str, ...) do { \
    error = strmake(str, ##__VA_ARGS__); \
    goto QUIT; \
} while (0);

#define PARSE_STR(str, var) *(var) = strdup(str)
#define PARSE_INT(str, var) do { \
    if (!strtoint(str, var)) { \
        ABORT_PARSE("integer conversion failed ('%s')", str); \
    } \
} while (0)

    model_parser_line_handler_arg *m_arg = (model_parser_line_handler_arg *) arg;
    model_parser *parser = m_arg->parser;
    model_parser_state *state = (model_parser_state *) parser->_state;
    model *model = m_arg->model;

    char **fields = NULL;
    char *error = NULL;
    char *line_copy1 = strdup(line);
    char *line_copy2 = strdup(line);

    // Outside section?
    fields = mallocx(2, sizeof(char *));
    int n_fields = strsplit(line_copy1, ":", fields, 2);
    if (n_fields > 2)
        ABORT_PARSE("unexpected line syntax '%s'", line);

    const char *key = n_fields >= 1 ? strtrim(fields[0]) : "";
    const char *value = n_fields == 2 ? strtrim(fields[1]) : "";

    // Header?
    if (streq("END.", key)) {
        goto QUIT;
    }
    if (streq("Name", key)) {
        PARSE_STR(value, &model->name);
        goto QUIT;
    }
    if (streq("Courses", key)) {
        PARSE_INT(value, &model->n_courses);
        goto QUIT;
    }
    if (streq("Rooms", key)) {
        PARSE_INT(value, &model->n_rooms);
        goto QUIT;
    }
    if (streq("Days", key)) {
        PARSE_INT(value, &model->n_days);
        goto QUIT;
    }
    if (streq("Periods_per_day", key)) {
        PARSE_INT(value, &model->n_slots);
        goto QUIT;
    }
    if (streq("Curricula", key)) {
        PARSE_INT(value, &model->n_curriculas);
        goto QUIT;
    }
    if (streq("Constraints", key)) {
        PARSE_INT(value, &model->n_unavailability_constraints);
        goto QUIT;
    }

    // Sections begin?
    if (streq("COURSES", key) && strempty(value)) {
        state->section = MODEL_PARSER_SECTION_COURSES;
        state->section_cursor = 0;
        model->courses = mallocx(model->n_courses, sizeof(course));
        goto QUIT;
    }
    if (streq("ROOMS", key) && strempty(value)) {
        state->section = MODEL_PARSER_SECTION_ROOMS;
        state->section_cursor = 0;
        model->rooms = mallocx(model->n_rooms, sizeof(course));
        goto QUIT;
    }
    if (streq("CURRICULA", key) && strempty(value)) {
        state->section = MODEL_PARSER_SECTION_CURRICULAS;
        state->section_cursor = 0;
        model->curriculas = mallocx(model->n_curriculas, sizeof(course));
        goto QUIT;
    }
    if (streq("UNAVAILABILITY_CONSTRAINTS", key) && strempty(value)) {
        state->section = MODEL_PARSER_SECTION_CONSTRAINTS;
        state->section_cursor = 0;
        model->unavailability_constraints =
                mallocx(model->n_unavailability_constraints, sizeof(course));
        goto QUIT;
    }

    free(fields);
    fields = NULL;

    // Inside section?
    if (state->section == MODEL_PARSER_SECTION_COURSES) {
        if (state->section_cursor >= model->n_courses)
            ABORT_PARSE("unexpected courses count");

        fields = mallocx(COURSE_FIELDS, sizeof(char *));
        if (strsplit(line_copy2, FIELDS_SEPARATORS, fields, COURSE_FIELDS) != COURSE_FIELDS)
            ABORT_PARSE("unexpected course entry syntax ('%s')", line);

        int f = 0;

        course *c = &model->courses[state->section_cursor];
        c->index = state->section_cursor;
        PARSE_STR(fields[f++], &c->id);
        PARSE_STR(fields[f++], &c->teacher_id);
        PARSE_INT(fields[f++], &c->n_lectures);
        PARSE_INT(fields[f++], &c->min_working_days);
        PARSE_INT(fields[f++], &c->n_students);

        state->section_cursor++;
        goto QUIT;
    }
    if (state->section == MODEL_PARSER_SECTION_ROOMS) {
        if (state->section_cursor >= model->n_rooms)
            ABORT_PARSE("unexpected rooms count");

        fields = mallocx(ROOM_FIELDS, sizeof(char *));
        if (strsplit(line_copy2, FIELDS_SEPARATORS, fields, ROOM_FIELDS) != ROOM_FIELDS)
            ABORT_PARSE("unexpected room entry syntax ('%s')", line);

        int f = 0;

        room *r = &model->rooms[state->section_cursor];
        r->index = state->section_cursor;
        PARSE_STR(fields[f++], &r->id);
        PARSE_INT(fields[f++], &r->capacity);

        state->section_cursor++;
        goto QUIT;
    }
    if (state->section == MODEL_PARSER_SECTION_CURRICULAS) {
        if (state->section_cursor >= model->n_curriculas)
            ABORT_PARSE("unexpected curriculas count");

        // Don't know in advance how many fields we will have
        // Therefore alloc at least 2 + n_courses
        fields = mallocx(CURRICULA_FIXED_FIELDS + model->n_courses, sizeof(char *));
        n_fields = strsplit(line_copy2, FIELDS_SEPARATORS, fields,
                                CURRICULA_FIXED_FIELDS + model->n_courses);

        if (n_fields < CURRICULA_FIXED_FIELDS) 
            ABORT_PARSE("unexpected course entry syntax ('%s')", line);
        
        int f = 0;
        
        curricula *q = &model->curriculas[state->section_cursor];
        q->index = state->section_cursor;
        PARSE_STR(fields[f++], &q->id);
        PARSE_INT(fields[f++], &q->n_courses);

        if (q->n_courses != n_fields - CURRICULA_FIXED_FIELDS)
            ABORT_PARSE("unexpected curricula fields count");

        q->courses_ids = mallocx(q->n_courses, sizeof(char *));
        for (int i = 0; i < q->n_courses; i++)
            PARSE_STR(fields[f++], &q->courses_ids[i]);

        state->section_cursor++;
        goto QUIT;
    }
    if (state->section == MODEL_PARSER_SECTION_CONSTRAINTS) {
        if (state->section_cursor >= model->n_unavailability_constraints)
            ABORT_PARSE("unexpected constraints count");

        fields = mallocx(UNAVAILABILITY_CONSTRAINTS_FIELDS, sizeof(char *));
        if (strsplit(line_copy2, FIELDS_SEPARATORS, fields, UNAVAILABILITY_CONSTRAINTS_FIELDS)
                != UNAVAILABILITY_CONSTRAINTS_FIELDS)
            ABORT_PARSE("unexpected constraint entry syntax ('%s')", line);


        int f = 0;

        unavailability_constraint *uc =
            &model->unavailability_constraints[state->section_cursor];
        PARSE_STR(fields[f++], &uc->course_id);
        PARSE_INT(fields[f++], &uc->day);
        PARSE_INT(fields[f++], &uc->slot);

        state->section_cursor++;
        goto QUIT;
    }

    eprint("WARN: unexpected line '%s'", line);

QUIT:
    free(line_copy1);
    free(line_copy2);
    free(fields);

#undef ABORT_PARSE
#undef PARSE_STR
#undef PARSE_INT

    return error;
}

bool model_parser_parse(model_parser *parser, const char *filename, model *model) {
    model_parser_line_handler_arg arg = {
        .parser = parser,
        .model = model
    };

    verbose("Going to parse model '%s'", filename);
    solution_parser_state_init(parser->_state);
    parser->error = fileparse(filename, NULL, model_parser_line_handler, &arg);
    solution_parser_state_destroy(parser->_state);

    bool success = strempty(parser->error);
    if (success) {
        model->_filename = filename;
        verbose("Model '%s' parsed successfully.\n"
                 " Courses: %d\n"
                 " Rooms: %d\n"
                 " Days: %d\n"
                 " Slots: %d\n"
                 " Curriculas: %d",
                 filename,
                 model->n_courses,
                 model->n_rooms,
                 model->n_days,
                 model->n_slots,
                 model->n_curriculas);
        model_finalize(model);
        verbose(" Lectures: %d", model->n_lectures);

    }
    return success;
}

#define ABORT_PARSE(errfmt, ...) do { \
    snprintf(error_reason, MAX_ERROR_LENGTH, errfmt, ##__VA_ARGS__); \
    goto QUIT; \
} while(0)

const char *model_parser_get_error(model_parser *parser) {
    return parser->error;
}
