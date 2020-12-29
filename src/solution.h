#ifndef SOLUTION_H
#define SOLUTION_H

#include <glib.h>
#include "model.h"

typedef struct assignment {
    course *course;
    room *room;
    int day;
    int day_period;
} assignment;

typedef struct solution {
    GList *assignments;
} solution;

assignment * assignment_new(course *course, room *room, int day, int day_period);
void assignment_set(assignment *a, course *course, room *room, int day, int day_period);

void solution_init(solution *solution);
void solution_destroy(solution *solution);

char * solution_to_string(const solution *sol);
char * solution_print(const solution *sol);

void solution_add_assignment(solution *sol, assignment *a);

#endif // SOLUTION_H