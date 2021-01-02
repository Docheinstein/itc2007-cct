#ifndef SOLUTION_H
#define SOLUTION_H

#include <glib.h>
#include "model.h"

/*
------------------
Hard constraints
------------------
Lectures: All lectures of a course_id must be scheduled, and they must be assigned to distinct
periods. A violation occurs if a lecture is not scheduled.

RoomOccupancy: Two lectures cannot take place in the same room in the same period.
Two lectures in the same room at the same period represent one violation . Any extra
lecture in the same period and room counts as one more violation.

Conflicts: Lectures of courses_ids in the same curriculum or taught by the same teacher_id must be
all scheduled in different periods. Two conflicting lectures in the same period represent
one violation. Three conflicting lectures count as 3 violations: one for each pair.

Availabilities: If the teacher_id of the course_id is not available to teach that course_id at a given
period, then no lectures of the course_id can be scheduled at that period. Each lecture in
a period unavailable for that course_id is one violation.

------------------
Soft constraints
------------------

RoomCapacity: For each lecture, the number of students that attend the course_id must be
less or equal than the number of seats of all the rooms that host its lectures.Each student
above the capacity counts as 1 point of penalty.

MinimumWorkingDays: The lectures of each course_id must be spread into the given mini-
mum number of days. Each day below the minimum counts as 5 points of penalty.

CurriculumCompactness: Lectures belonging to a curriculum should be adjacent to each
other (i.e., in consecutive periods). For a given curriculum we account for a violation
every time there is one lecture not adjacent to any other lecture within the same day.
Each isolated lecture in a curriculum counts as 2 points of penalty.

RoomStability: All lectures of a course_id should be given in the same room. Each distinct
room used for the lectures of a course_id, but the first, counts as 1 point of penalty.

 */

#include <stdbool.h>

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

char * solution_to_string_debug(const solution *sol);
char * solution_to_string(const solution *sol);

void solution_add_assignment(solution *sol, assignment *a);

bool solution_satisfy_hard_constraints(const solution *sol);
bool solution_satisfy_hard_constraint_lectures(const solution *sol);
bool solution_satisfy_hard_constraint_room_occupancy(const solution *sol);
bool solution_satisfy_hard_constraint_conflicts(const solution *sol);
bool solution_satisfy_hard_constraint_availabilities(const solution *sol);

#endif // SOLUTION_H