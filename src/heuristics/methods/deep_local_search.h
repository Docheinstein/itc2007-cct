#ifndef deep_local_search_H
#define deep_local_search_H

#include "heuristics/heuristic_solver.h"

/*
 * Deep Local Search.
 * Try every move for every neighbour at distance 1 accepting
 * the move pair only if decrease the cost of the solution.
 */

typedef struct deep_local_search_params {
    double max_distance_from_best_ratio;
} deep_local_search_params;

void deep_local_search_params_default(deep_local_search_params *params);

void deep_local_search(heuristic_solver_state *state, void *arg);

#endif // deep_local_search_H