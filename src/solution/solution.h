#ifndef SOLUTION_H
#define SOLUTION_H

#include <glib.h>
#include "model/model.h"
#include <stdio.h>

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

extern const int ROOM_CAPACITY_COST_FACTOR;
extern const int MIN_WORKING_DAYS_COST_FACTOR;
extern const int CURRICULUM_COMPACTNESS_COST_FACTOR;
extern const int ROOM_STABILITY_COST_FACTOR;

typedef struct assignment {
    int r, d, s;
} assignment;

typedef struct solution {
    const model *model;

    bool *timetable_crds;
    bool *timetable_cdsr;
    bool *timetable_rdsc;
    bool *timetable_qdscr;
    bool *timetable_tdscr;

    int *c_rds;
    int *r_cds;
    int *l_rds;

    int *sum_cr;
    int *sum_cds;
    int *sum_cd;
    int *sum_rds;
    int *sum_qds;
    int *sum_tds;

    assignment *assignments;

    int _id;
} solution;


void solution_init(solution *solution, const model *model);
void solution_clear(solution *solution);
void solution_destroy(solution *solution);

void solution_copy(solution *solution_dest, const solution *solution_src);

void solution_assign_lecture(solution *sol, int l1, int r2, int d2, int s2);
void solution_unassign_lecture(solution *sol, int l);
void solution_get_lecture_assignment(const solution *sol, int l, int *r, int *d, int *s);

char * solution_to_string(const solution *sol);
char * solution_quality_to_string(const solution *sol, bool verbose);
bool write_solution(const solution *sol, const char *filename);

// Hard constraints
bool solution_satisfy_hard_constraints(const solution *sol);

bool solution_satisfy_lectures(const solution *sol);
bool solution_satisfy_room_occupancy(const solution *sol);
bool solution_satisfy_conflicts(const solution *sol);
bool solution_satisfy_availabilities(const solution *sol);

int solution_lectures_violations(const solution *sol);
int solution_room_occupancy_violations(const solution *sol);
int solution_conflicts_violations(const solution *sol);
int solution_availabilities_violations(const solution *sol);

// Soft constraints
int solution_cost(const solution *sol);
int solution_room_capacity_cost(const solution *sol);
int solution_min_working_days_cost(const solution *sol);
int solution_curriculum_compactness_cost(const solution *sol);
int solution_room_stability_cost(const solution *sol);

// Debug purpose
unsigned long long solution_fingerprint(const solution *sol);
void solution_assert(const solution *sol, bool expected_feasibility, int expected_cost);
void solution_assert_consistency(const solution *sol);

#endif // SOLUTION_H