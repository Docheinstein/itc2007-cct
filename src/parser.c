#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "log/debug.h"
#include "log/verbose.h"
#include "utils.h"
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
    <curricula> := <CurriculumID> <# Courses> <MemberID> ... <MemberID>
    <unavailability_constraint> := <CourseID> <Day> <Day_Period>
*/

#define SECTION_NONE            0
#define SECTION_COURSES         1
#define SECTION_ROOMS           2
#define SECTION_CURRICULA       3
#define SECTION_CONSTRAINTS     4

#define DELIMITERS " \t"
#define COURSE_FIELDS 5

bool parse(char *input, model *model) {
    verbose("Opening input file: '%s'", input);

    FILE *file = fopen(input, "r");

    if (!file) {
        eprint("ERROR: failed to open '%s' (%s)\n", input, strerror(errno));
        return false;
    }

    int line_num = 0;
    char line[256];

    char key_[256];
    char value_[256];

    int section = SECTION_NONE;
    int section_entry = 0;

    bool ok = true;

    while (fgets(line, LENGTH(line), file) && ok) {
        ++line_num;
        line[strlen(line) - 1] = '\0';
        verbose("<< %s", line);

        // Inside section?
        if (section == SECTION_COURSES) {
            char *course_tokens[COURSE_FIELDS];
            if (strsplit(line, DELIMITERS, course_tokens, COURSE_FIELDS) == COURSE_FIELDS) {
                course *c = &model->courses[section_entry++];
                course_init(c);
                int f = 0;
                c->id = strdup(course_tokens[f++]);
                c->teacher = strdup(course_tokens[f++]);
                c->n_lectures = strtoint(course_tokens[f++], &ok); if (!ok) break;
                c->min_working_days = strtoint(course_tokens[f++], &ok); if (!ok) break;
                c->n_students = strtoint(course_tokens[f++], &ok); if (!ok) break;
                continue;
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

            debug("(key='%s', value='%s')", key, value);

            // Preliminary fields
            if (streq("Name", key))
                model->name = strdup(value);
            else if (streq("Courses", key))
                model->n_courses = strtoint(value, &ok);
            else if (streq("Rooms", key))
                model->n_rooms = strtoint(value, &ok);
            else if (streq("Days", key))
                model->n_days = strtoint(value, &ok);
            else if (streq("Periods_per_day", key))
                model->n_periods_per_day = strtoint(value, &ok);
            else if (streq("Curricula", key))
                model->n_curricula = strtoint(value, &ok);
            else if (streq("Constraints", key))
                model->n_unavailability_constraints = strtoint(value, &ok);

            // Sections begin
            else if (streq("COURSES", key) && strempty(value)) {
                section = SECTION_COURSES;
                model->courses = malloc(sizeof(course) * model->n_courses);
            }
            else if (streq("ROOMS", key) && strempty(value)) {
                section = SECTION_ROOMS;
                model->rooms = malloc(sizeof(course) * model->n_rooms);
            }
            else if (streq("CURRICULA", key) && strempty(value)) {
                section = SECTION_CURRICULA;
                model->curriculas = malloc(sizeof(course) * model->n_curricula);
            }
            else if (streq("UNAVAILABILITY_CONSTRAINTS", key) && strempty(value)) {
                section = SECTION_CONSTRAINTS;
                model->unavailability_constraints =
                        malloc(sizeof(course) * model->n_unavailability_constraints);
            }
        }
    }

    verbose("Closing file: '%s'", input);
    if (fclose(file) != 0)
        eprint("WARN: failed to close '%s' (%s)\n", input, strerror(errno));

    if (!ok)
        eprintf("ERROR: failed to parse input file, error at line %d\n"
               "%s", line_num, line);

    return ok;
}