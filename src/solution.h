#ifndef SOLUTION_H
#define SOLUTION_H

#include <glib.h>
#include "model.h"
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

typedef struct solution_helper {
    bool valid;

    int *c_rds;
    int *r_cds;

    int *sum_cr;

    bool *timetable_cdsr;
    int *sum_cds;
    int *sum_cd;

    bool *timetable_rdsc;
    int *sum_rds;

    bool *timetable_qdscr;
    int *sum_qds;

    bool *timetable_tdscr;
    int *sum_tds;
} solution_helper;

typedef struct solution {
    bool *timetable;
    const model *model;

    solution_helper *helper;
} solution;


typedef struct solution_parser {
    char *error;
} solution_parser;

typedef struct solution_fingerprint_t {
    unsigned long long sum;
    unsigned long long xor;
} solution_fingerprint_t;

bool read_solution(solution *sol, const char *input_file);
bool write_solution(const solution *sol, const char *output_file);
bool write_solution_timetable(const solution *sol, const char *output_file);
void print_solution(const solution *sol, FILE *stream);
void print_solution_full(const solution *sol, FILE *stream);

void solution_parser_init(solution_parser *solution_parser);
bool solution_parser_parse(solution_parser *solution_parser, const char * input,
                           solution *solution);
void solution_parser_destroy(solution_parser *solution_parser);
const char * solution_parser_get_error(solution_parser *solution_parser);

void solution_init(solution *solution, const model *model);
void solution_reinit(solution *solution);
void solution_destroy(solution *solution);


const solution_helper * solution_get_helper(solution *solution);
bool solution_invalidate_helper(solution *sol);
bool solution_helper_equal(solution *s1, solution *s2);
bool solution_helper_equal_0(const solution_helper *s1, const solution_helper *s2, const model *model);

void solution_copy(solution *solution_dest, const solution *solution_src);

void solution_set(solution *sol, int c, int r, int d, int s, bool value);
bool solution_get(const solution *sol, int c, int r, int d, int s);

void solution_set_at(solution *sol, int index, bool value);
bool solution_get_at(const solution *sol, int index);

char * solution_to_string(const solution *sol);
char * solution_quality_to_string(const solution *sol, bool verbose);

char * solution_timetable_to_string(const solution *sol);

void solution_fingerprint_init(solution_fingerprint_t *f);
bool solution_equal(const solution *s1, const solution *s2);
bool solution_fingerprint_equal(solution_fingerprint_t f1, solution_fingerprint_t f2);
void solution_fingerprint_add(solution_fingerprint_t *f, int i);
void solution_fingerprint_sub(solution_fingerprint_t *f, int i);

solution_fingerprint_t solution_fingerprint(const solution *sol);
int solution_assignment_count(const solution *sol);

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

int solution_room_capacity_lecture_cost(solution *sol, int c, int r, int d, int s);
int solution_min_working_days_course_cost(solution *sol, int c);
int solution_curriculum_compactness_lecture_cost(solution *sol, int c, int r, int d, int s);
int solution_room_stability_course_cost(solution *sol, int c);

#endif // SOLUTION_H