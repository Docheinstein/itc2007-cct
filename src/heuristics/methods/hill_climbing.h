#ifndef HILL_CLIMBING_H
#define HILL_CLIMBING_H

#include "heuristics/heuristic_solver.h"

/*
 * Hill Climbing.
 * Generates a random move and performs it only if is does not
 * increase the cost of the current solution.
 *
 * `max_idle` defines after how many iterations of non-decreasing
 *      cost (side moves) the method quits.
 */

typedef struct hill_climbing_params {
    long max_idle;
} hill_climbing_params;

void hill_climbing_params_default(hill_climbing_params *params);

void hill_climbing(heuristic_solver_state *state, void *arg);

#endif // HILL_CLIMBING_H