#ifndef NEIGHBOURHOOD_SWAP_H
#define NEIGHBOURHOOD_SWAP_H

#include "solution.h"
#include "neighbourhood.h"

typedef struct neighbourhood_swap_move {
    int c1, r1, d1, s1;
    int r2, d2, s2;

    int _c2; // implicit
} neighbourhood_swap_move;

typedef struct neighbourhood_swap_iter {
    solution *solution;
    bool end;
    int rds_index;
    neighbourhood_swap_move current;
} neighbourhood_swap_iter;

typedef struct neighbourhood_swap_result {
    bool feasible;
    int delta_cost;
    // TODO: remove, keep just for debug
    int delta_cost_room_capacity;
    int delta_cost_min_working_days;
    int delta_cost_curriculum_compactness;
    int delta_cost_room_stability;
} neighbourhood_swap_result;

void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, solution *sol);
void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter);
bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, neighbourhood_swap_move *move);

void neighbourhood_swap_predict(solution *sol, const neighbourhood_swap_move *move,
                                neighbourhood_swap_result *result);
bool neighbourhood_swap(solution *sol,
                        neighbourhood_swap_move *move,
                        neighbourhood_prediction_strategy predict_feasibility,
                        neighbourhood_prediction_strategy predict_cost,
                        neighbourhood_performing_strategy perform,
                        neighbourhood_swap_result *result);

void neighbourhood_swap_back(solution *sol,
                             const neighbourhood_swap_move *move);


#endif // NEIGHBOURHOOD_SWAP_H