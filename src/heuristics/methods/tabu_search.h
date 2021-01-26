#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H

#include "heuristics/heuristic_solver.h"

typedef struct tabu_search_params {
    int tabu_tenure;
    int max_idle;
    double frequency_penalty_coeff;
    bool random_pick;
    bool steepest;
    bool clear_on_best;
} tabu_search_params;

void tabu_search(heuristic_solver_state *state, void *arg);

#endif // TABU_SEARCH_H