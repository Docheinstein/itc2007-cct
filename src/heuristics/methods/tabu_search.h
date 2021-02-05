#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H

#include "heuristics/heuristic_solver.h"

/*
 * Tabu Search.
 * Performs the best possible move of the neighbourhood
 * (chosen at random if more than one exists).
 * After that, the (room, day, slot) assignment of the courses
 * moved by the swap move is stored into the tabu_list and therefore
 * the lectures of these courses cannot came back to the same
 * (room, day, slot) within the  next `tabu_tenure` iterations.
 *
 * `frequency_penalty_coeff` increases the ban time of a move
 *      by `frequency_penalty_coeff` * count(move)
 */

typedef struct tabu_search_params {
    long max_idle;
    double max_idle_near_best_coeff;
    double near_best_ratio;
    int tabu_tenure;
    double frequency_penalty_coeff;
} tabu_search_params;

void tabu_search_params_default(tabu_search_params *params);

void tabu_search(heuristic_solver_state *state, void *arg);

#endif // TABU_SEARCH_H