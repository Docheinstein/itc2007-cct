#ifndef NEIGHBOURHOOD_SWAP_H
#define NEIGHBOURHOOD_SWAP_H

#include "solution.h"
#include "neighbourhood.h"

typedef struct neighbourhood_swap_move {
    int c1, r1, d1, s1;
    int c2, r2, d2, s2;
    int _c2;
    int l1, l2;
} neighbourhood_swap_move;

typedef struct neighbourhood_swap_iter {
    solution *solution;
    bool end;
    int rds_index;
    neighbourhood_swap_move current;
    int i;
    int lecture_index;
} neighbourhood_swap_iter;

typedef struct neighbourhood_swap_result {
    solution_fingerprint_t fingerprint_plus;
    solution_fingerprint_t fingerprint_minus;
    bool _helper_was_valid;
    bool feasible;
    int delta_cost;
    // TODO: remove, keep just for debug
    int delta_cost_room_capacity;
    int delta_cost_min_working_days;
    int delta_cost_curriculum_compactness;
    int delta_cost_room_stability;
} neighbourhood_swap_result;

int neighbourhood_swap_move_compare_cost(const neighbourhood_swap_result *res1,
                                         const neighbourhood_swap_result *res2);

int neighbourhood_swap_move_compare_room_capacity_cost(
        const neighbourhood_swap_result *res1,
        const neighbourhood_swap_result *res2);

int neighbourhood_swap_move_compare_room_min_working_days_cost(
        const neighbourhood_swap_result *res1,
        const neighbourhood_swap_result *res2);

int neighbourhood_swap_move_compare_room_curriculum_compactness_cost(
        const neighbourhood_swap_result *res1,
        const neighbourhood_swap_result *res2);

int neighbourhood_swap_move_compare_room_stability_cost(
        const neighbourhood_swap_result *res1,
        const neighbourhood_swap_result *res2);

bool neighbourhood_swap_move_equal(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2);
int neighbourhood_swap_move_cost(const neighbourhood_swap_move *move, solution *s);

void neighbourhood_swap_generate_random_move(solution *sol, neighbourhood_swap_move *mv);

void neighbourhood_swap_move_copy(neighbourhood_swap_move *dest,
                                  const neighbourhood_swap_move *src);

void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, solution *sol);
void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter);
bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, neighbourhood_swap_move *move);

void neighbourhood_swap_predict(solution *sol, const neighbourhood_swap_move *move,
                                neighbourhood_swap_result *result);
bool neighbourhood_swap(solution *sol,
                        neighbourhood_swap_move *move,
                        neighbourhood_prediction_strategy predict_feasibility,
                        neighbourhood_prediction_strategy predict_cost,
                        neighbourhood_prediction_strategy predict_fingerprint,
                        neighbourhood_performing_strategy perform,
                        neighbourhood_swap_result *result);

void neighbourhood_swap_back(solution *sol,
                             const neighbourhood_swap_move *move,
                             const neighbourhood_swap_result *result,
                             bool revalidate);


bool neighbourhood_swap_move_same_room(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2);
bool neighbourhood_swap_move_same_period(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2);
bool neighbourhood_swap_move_same_course(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2);

#endif // NEIGHBOURHOOD_SWAP_H