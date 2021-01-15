#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

#include "solution.h"

typedef struct neighbourhood_swap_iter {
    solution *solution;
    bool end;
    int rds_index;
    int c1, r1, d1, s1;
    int r2, d2, s2;
} neighbourhood_swap_iter;

typedef struct neighbourhood_swap_result {
    int c2;
    bool feasible;
    int delta_cost;
    // TODO: remove, keep just for debug
    int delta_cost_room_capacity;
    int delta_cost_min_working_days;
    int delta_cost_curriculum_compactness;
    int delta_cost_room_stability;
} neighbourhood_swap_result;

typedef enum neighbourhood_prediction_strategy {
    NEIGHBOURHOOD_PREDICT_ALWAYS,
    NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
    NEIGHBOURHOOD_PREDICT_NEVER
} neighbourhood_prediction_strategy;

typedef enum neighbourhood_performing_strategy {
    NEIGHBOURHOOD_PERFORM_ALWAYS,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE,
    NEIGHBOURHOOD_PERFORM_IF_BETTER,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
    NEIGHBOURHOOD_PERFORM_NEVER,
} neighbourhood_performing_strategy;

void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, solution *sol);
void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter);
bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, int *c1,
                                  int *r1, int *d1, int *s1,
                                  int *r2, int *d2, int *s2);
void neighbourhood_swap_predict(solution *sol,
                        int c1, int r1, int d1, int s1, int r2, int d2, int s2,
                        neighbourhood_swap_result *result);
bool neighbourhood_swap(solution *sol,
                        int c1, int r1, int d1, int s1, int r2, int d2, int s2,
                        neighbourhood_prediction_strategy predict_feasibility,
                        neighbourhood_prediction_strategy predict_cost,
                        neighbourhood_performing_strategy perform,
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