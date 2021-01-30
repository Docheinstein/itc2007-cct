#ifndef SWAP_H
#define SWAP_H

#include "solution/solution.h"
#include "neighbourhood.h"

typedef struct swap_move {
    int l1;          // lecture (from)
    int r2, d2, s2;  // assignment (to)
    struct {
        int c1, r1, d1, s1;
        int l2, c2;
    } helper;
} swap_move;

typedef struct swap_iter {
    const solution *solution;
    swap_move current;
    bool end;
    int i;
    int ii;
} swap_iter;

typedef struct swap_result {
    bool feasible;
    struct {
        int cost;
        int room_capacity_cost;
        int min_working_days_cost;
        int curriculum_compactness_cost;
        int room_stability_cost;
    } delta;
} swap_result;

void swap_iter_init(swap_iter *iter, const solution *sol);
void swap_iter_destroy(swap_iter *iter);
bool swap_iter_next(swap_iter *iter, swap_move *move);

bool swap_move_is_effective(const swap_move *mv);
void swap_move_generate_random_raw(const solution *sol, swap_move *mv);
void swap_move_generate_random(const solution *sol, swap_move *mv, bool require_feasible);
void swap_move_copy(swap_move *dest, const swap_move *src);

void swap_predict(const solution *sol, const swap_move *move,
                  neighbourhood_predict_strategy predict_feasibility,
                  neighbourhood_predict_strategy predict_cost,
                  swap_result *result);
bool swap_perform(solution *sol, const swap_move *move,
                  neighbourhood_perform_strategy perform,
                  swap_result *result);

bool swap_extended(solution *sol, const swap_move *move,
                   neighbourhood_predict_strategy predict_feasibility,
                   neighbourhood_predict_strategy predict_cost,
                   neighbourhood_perform_strategy perform,
                   swap_result *result);

#endif // SWAP_H