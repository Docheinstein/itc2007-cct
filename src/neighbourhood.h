#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

#include "solution.h"

typedef struct neighbourhood_swap_iter {
    const solution *solution;
    bool begin, end;
    int c1, r1, d1, s1;
    int r2, d2, s2;
} neighbourhood_swap_iter;

typedef struct neighbourhood_swap_result {
    int c2;
    bool feasible;
    int delta_cost;
    int delta_cost_room_capacity;
    int delta_cost_min_working_days;
    int delta_cost_curriculum_compactness;
    int delta_cost_room_stability;
} neighbourhood_swap_result;

void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, const solution *sol);
void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter);
bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, int *c1,
                                  int *r1, int *d1, int *s1,
                                  int *r2, int *d2, int *s2);
void neighbourhood_swap(solution *sol,
                        int c1, int r1, int d1, int s1, int r2, int d2, int s2,
                        neighbourhood_swap_result *result);

void neighbourhood_swap_back(solution *sol,
                             int c1, int r1, int d1, int s1,
                             int c2, int r2, int d2, int s2);

// -----------------------------------------------------------------------------

typedef struct neighbourhood_swap_course_lectures_room_iter {
    const solution *solution;
    int course_index;
    int room_index;
} neighbourhood_swap_course_lectures_room_iter;


void neighbourhood_swap_course_lectures_room_iter_init(
        neighbourhood_swap_course_lectures_room_iter *iter, const solution *sol);
void neighbourhood_swap_course_lectures_room_iter_destroy(
        neighbourhood_swap_course_lectures_room_iter *iter);
bool neighbourhood_swap_course_lectures_room_iter_next(
        neighbourhood_swap_course_lectures_room_iter *iter, int *c, int *r);
bool neighbourhood_swap_course_lectures_room(
        solution *sol_out, int c, int r);


#endif // NEIGHBOURHOOD_H