#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <io_utils.h>
#include <str_utils.h>
#include "verbose.h"
#include "model.h"
#include "mem_utils.h"
#include "array_utils.h"

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

#define MAX_ERROR_LENGTH        256
#define MAX_LINE_LENGTH         256

#define SECTION_NONE            0
#define SECTION_COURSES         1
#define SECTION_ROOMS           2
#define SECTION_CURRICULAS      3
#define SECTION_CONSTRAINTS     4

#define FIELDS_SEPARATORS                   " \t"
#define COURSE_FIELDS                       5
#define ROOM_FIELDS                         2
#define CURRICULA_FIXED_FIELDS              2
#define UNAVAILABILITY_CONSTRAINTS_FIELDS   3


void parser_init(parser *parser) {
    parser->error = NULL;
}

void parser_destroy(parser *parser) {
    free(parser->error);
}

const char *parser_get_error(parser *parser) {
    return parser->error;
}

bool parser_parse(parser *parser, char *input, model *model) {

#define ABORT_PARSE(errfmt, ...) do { \
    snprintf(error_reason, MAX_ERROR_LENGTH, errfmt, ##__VA_ARGS__); \
    goto QUIT; \
} while(0)

#define ABORT_PARSE_INT_FAIL() ABORT_PARSE("integer conversion failed")

    char error_reason[MAX_ERROR_LENGTH];
    error_reason[0] = '\0';
    int line_num = 0;

    char line[MAX_LINE_LENGTH];
    char line_copy[MAX_LINE_LENGTH];

    char key_[MAX_LINE_LENGTH];
    char value_[MAX_LINE_LENGTH];
    // ??? needed ???
//    key_[MAX_LINE_LENGTH - 1] = '\0';
//    value_[MAX_LINE_LENGTH - 1] = '\0';


    int section = SECTION_NONE;
    int section_entry = 0;

    bool ok = true;

    verbose("Opening input file: '%s'", input);

    FILE *file = fopen(input, "r");

    if (!file)
        ABORT_PARSE("failed to open '%s' (%s)\n", input, strerror(errno));

    while (ok && fgets(line, LENGTH(line), file)) {
        ++line_num;
        line[strlen(line) - 1] = '\0';
        verbose("<< %s", line);

        if (strempty(line))
            continue;

        // Make a copy since strsplit() modifies the original line
        snprintf(line_copy, MAX_LINE_LENGTH, "%s", line);

        // Inside section?
        if (section == SECTION_COURSES) {
            char *tokens[COURSE_FIELDS];
            if (strsplit(line, FIELDS_SEPARATORS, tokens, COURSE_FIELDS) == COURSE_FIELDS) {
                if (section_entry >= model->n_courses)
                    ABORT_PARSE("unexpected courses count");

                course *c = &model->courses[section_entry++];
                int f = 0;
                c->id = strdup(tokens[f++]);
                c->teacher_id = strdup(tokens[f++]);
                c->n_lectures = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
                c->min_working_days = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
                c->n_students = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
                continue;
            }
        } else if (section == SECTION_ROOMS) {
            char *tokens[ROOM_FIELDS];
            if (strsplit(line, FIELDS_SEPARATORS, tokens, ROOM_FIELDS) == ROOM_FIELDS) {
                if (section_entry >= model->n_rooms)
                    ABORT_PARSE("unexpected rooms count");

                room *r = &model->rooms[section_entry++];
                int f = 0;
                r->id = strdup(tokens[f++]);
                r->capacity = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
            }
        } else if (section == SECTION_CURRICULAS) {
            // Don't know in advance how many tokens we will have
            // Therefore alloc at least 2 + n_courses
            char *tokens[CURRICULA_FIXED_FIELDS + model->n_courses];
            int n_tokens = strsplit(line, FIELDS_SEPARATORS, tokens,
                                    CURRICULA_FIXED_FIELDS + model->n_courses);
            if (n_tokens >= CURRICULA_FIXED_FIELDS) {
                if (section_entry >= model->n_curriculas)
                    ABORT_PARSE("unexpected curriculas count");

                curricula *q = &model->curriculas[section_entry++];
                int f = 0;
                q->id = strdup(tokens[f++]);
                q->n_courses = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();

                if (q->n_courses != n_tokens - CURRICULA_FIXED_FIELDS)
                    ABORT_PARSE("unexpected curricula fields count");

                q->courses_ids = mallocx(sizeof(char *) * q->n_courses);
                for (int i = 0; i < q->n_courses; i++) {
                    q->courses_ids[i] = strdup(tokens[f++]);
                }
            }
        } else if (section == SECTION_CONSTRAINTS) {
            char *tokens[UNAVAILABILITY_CONSTRAINTS_FIELDS];
            if (strsplit(line, FIELDS_SEPARATORS, tokens, UNAVAILABILITY_CONSTRAINTS_FIELDS)
                    == UNAVAILABILITY_CONSTRAINTS_FIELDS) {
                if (section_entry >= model->n_unavailability_constraints)
                    ABORT_PARSE("unexpected unavailability constraints count");

                unavailability_constraint *uc =
                        &model->unavailability_constraints[section_entry++];
                int f = 0;
                uc->course_id = strdup(tokens[f++]);
                uc->day = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
                uc->slot = strtoint(tokens[f++], &ok); if (!ok) ABORT_PARSE_INT_FAIL();
            }
        }

        // Outside section?
        int colon = strpos(line, ':');

        if (colon > 0) {
            strncpy(key_, line, colon);
            key_[colon] = '\0';

            strncpy(value_, &line[colon + 1], strlen(line) - colon - 1);
            value_[strlen(line) - colon - 1] = '\0';

            char *key = strtrim(key_);
            char *value = strtrim(value_);
//            char *value = value_;

            // Preliminary fields
            if (streq("Name", key))
                model->name = strdup(value);
            else if (streq("Courses", key)) {
                model->n_courses = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }
            else if (streq("Rooms", key)) {
                model->n_rooms = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }
            else if (streq("Days", key)) {
                model->n_days = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }
            else if (streq("Periods_per_day", key)) {
                model->n_slots = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }
            else if (streq("Curricula", key)) {
                model->n_curriculas = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }
            else if (streq("Constraints", key)) {
                model->n_unavailability_constraints = strtoint(value, &ok); if (!ok) ABORT_PARSE_INT_FAIL(); }

            // Sections begin
            else if (streq("COURSES", key) && strempty(value)) {
                section = SECTION_COURSES;
                section_entry = 0;
                model->courses = mallocx(sizeof(course) * model->n_courses);
            }
            else if (streq("ROOMS", key) && strempty(value)) {
                section = SECTION_ROOMS;
                section_entry = 0;
                model->rooms = mallocx(sizeof(course) * model->n_rooms);
            }
            else if (streq("CURRICULA", key) && strempty(value)) {
                section = SECTION_CURRICULAS;
                section_entry = 0;
                model->curriculas = mallocx(sizeof(course) * model->n_curriculas);
            }
            else if (streq("UNAVAILABILITY_CONSTRAINTS", key) && strempty(value)) {
                section = SECTION_CONSTRAINTS;
                section_entry = 0;
                model->unavailability_constraints =
                        mallocx(sizeof(course) * model->n_unavailability_constraints);
            }
        }
    }

    verbose("Closing file: '%s'", input);
    if (fclose(file) != 0)
        verbose("WARN: failed to close '%s' (%s)", input, strerror(errno));

QUIT:
    if (!strempty(error_reason))
        parser->error = strmake("parse error at line %d (%s)", line_num, error_reason);

    return !parser->error;
}
