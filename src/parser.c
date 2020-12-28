#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
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

bool parse(char *input, model *model) {
    verbose("Opening input file: '%s'", input);

    FILE *file = fopen(input, "r");

    if (!file) {
        eprint("ERROR: failed to open '%s' (%s)\n", input, strerror(errno));
        return false;
    }

    int line_num = 1;
    char line[256];

    char key_[256];
    char value_[256];

    bool ok = true;

    while (fgets(line, LENGTH(line), file)) {
        verbosef("<< %s", line);
        int colon = strpos(line, ':');

        if (colon > 0) {
            strncpy(key_, line, colon);
            key_[colon] = '\0';

            strncpy(value_, &line[colon + 1], strlen(line) - colon - 2);
            value_[strlen(line) - colon - 2] = '\0';

            char *key = strtrim(key_);
            char *value = strtrim(value_);

            debug("(key='%s', value='%s')", key, value);

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
                model->n_constraints = strtoint(value, &ok);

            if (!ok)
                break;
        }

        ++line_num;
    }

    verbose("Closing file: '%s'", input);
    if (fclose(file) != 0)
        eprint("WARN: failed to close '%s' (%s)\n", input, strerror(errno));

    if (!ok)
        eprintf("ERROR: failed to parse input file, error at line %d\n"
               "%s", line_num, line);

    return ok;
}