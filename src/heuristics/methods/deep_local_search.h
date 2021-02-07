#ifndef DEEP_LOCAL_SEARCH_H
#define DEEP_LOCAL_SEARCH_H

#include "heuristics/heuristic_solver.h"

/*
 * Deep Local Search.
 * Try every move for each move at distance 1 accepting
 * the move pair only if the sum of the two
 * decrease the cost of the solution.
 *
 * It's very time consuming; it's not recommend in time-limited scenarios;
 * Can be useful to see if an obtained minimum it's a local minimum
 * even at depth 2.
 */

typedef struct deep_local_search_params {
    double max_distance_from_best_ratio;
} deep_local_search_params;

void deep_local_search_params_default(deep_local_search_params *params);

void deep_local_search(heuristic_solver_state *state, void *arg);

#endif // DEEP_LOCAL_SEARCH_H