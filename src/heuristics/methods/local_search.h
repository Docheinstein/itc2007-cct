#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "heuristics/heuristic_solver.h"

typedef struct local_search_params {
    bool steepest;
} local_search_params;

void local_search(heuristic_solver_state *state, void *arg);

#endif // LOCAL_SEARCH_H