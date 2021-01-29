#ifndef HILL_CLIMBING_H
#define HILL_CLIMBING_H

#include "heuristics/heuristic_solver.h"

typedef struct hill_climbing_params {
    int max_idle;
} hill_climbing_params;

void hill_climbing_params_default(hill_climbing_params *params);

void hill_climbing(heuristic_solver_state *state, void *arg);

#endif // HILL_CLIMBING_H