#ifndef SOLUTION_H
#define SOLUTION_H

#include "model/model.h"
#include <stdbool.h>

/**
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

/*
 * ITC2007 instances' solution.
 */

/* Penalty constants */
extern const int ROOM_CAPACITY_COST_FACTOR;
extern const int MIN_WORKING_DAYS_COST_FACTOR;
extern const int CURRICULUM_COMPACTNESS_COST_FACTOR;
extern const int ROOM_STABILITY_COST_FACTOR;

/*
 * Assignment of a lecture to (room, day, slot)
 */
typedef struct assignment {
    int r, d, s;
} assignment;

/*
 * Solution entity.
 * For represent a solution, only a timetable (e.g. timetable_crds) could
 * be enough, but for provide a more efficient computation, redundant
 * data is store (which must remain consistent).
 */
typedef struct solution {
    const model *model;

    /*
     * Timetables.
     * e.g. timetable_crds[c,r,d,s] is true if:
     *      course c is assigned to room r on day d, slot s
     */
    bool *timetable_crds;  // [c,r,d,s]
    bool *timetable_cdsr;  // [c,d,s,r]
    bool *timetable_rdsc;  // [r,d,s,c]
    bool *timetable_qdscr; // [q,d,s,c,r]
    bool *timetable_tdscr; // [t,d,s,c,r]

    /*
     * Course/Room/Lecture helpers.
     * e.g. c_rds[r,d,s] contains:
     *      the course c assigned to room r on day d, slot s
     *      or -1 if no course is assigned
     *
     */
    int *c_rds; // [r,d,s]
    int *r_cds; // [c,d,s]
    int *l_rds; // [r,d,s]

    /*
     * Sum helpers.
     * e.g. sum_cr[c,r] contains sum_d€D(sum_s€S(timetable_crds[c,r,d,s]))
     *      sum_cds[c,d,s] contains sum_r€R(timetable_crds[c,r,d,s])
     */
    int *sum_cr;    // [c,r]
    int *sum_cds;   // [c,d,s]
    int *sum_cd;    // [c,d]
    int *sum_rds;   // [r,d,s]
    int *sum_qds;   // [q,d,s]
    int *sum_tds;   // [t,d,s]

    // Assignment array of the lectures
    assignment *assignments; // [l]

    // Debug purposes
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

void solution_assert_consistency(const solution *sol);
void solution_assert_consistency_real(const solution *sol);
void solution_assert(const solution *sol, bool expected_feasibility, int expected_cost);
void solution_assert_real(const solution *sol, bool expected_feasibility, int expected_cost);

#endif // SOLUTION_H