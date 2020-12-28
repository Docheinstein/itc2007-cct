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

int parse(char *input, model *model) {
    verbose("Opening input file: '%s'", input);

    FILE *file = fopen(input, "r");

    if (!file)
        return errno;

    char line[128];

    char key[128];
    char value[128];

    while (fgets(line, LENGTH(line), file)) {
        verbosef("<< %s", line);
        int colon = strpos(line, ':');

        if (colon > 0) {
            strncpy(key, line, colon);
            key[colon] = '\0';

            strncpy(value, &line[colon + 1], strlen(line) - colon - 2);
            value[strlen(line) - colon - 2] = '\0';

            char *k = strtrim(key);
            char *v = strtrim(value);

            debug("(key='%s', value='%s')", k, v);

            if (streq("Name", key)) {
                model->name = strdup(value);
            } else if (streq("Courses", key)) {
                // ...
            }

            break;
        }
    }

    verbose("Closing file: '%s'", input);
    fclose(file);

    return 0;
}