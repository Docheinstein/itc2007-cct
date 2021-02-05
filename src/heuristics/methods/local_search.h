#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "heuristics/heuristic_solver.h"

/*
 * Local Search.
 * Performs the first seen improving move of the neighbourhood
 * until such a move exists, therefore reaches a local minimum.
 */

typedef struct local_search_params {
    double max_distance_from_best_ratio;
} local_search_params;

void local_search_params_default(local_search_params *params);

void local_search(heuristic_solver_state *state, void *arg);

#endif // LOCAL_SEARCH_H