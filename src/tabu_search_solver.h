#ifndef TABU_SEARCH_SOLVER_H
#define TABU_SEARCH_SOLVER_H


#include <stdbool.h>
#include "solution.h"

typedef struct tabu_search_solver_config {
    int time_limit;
    double difficulty_ranking_randomness;
} tabu_search_solver_config;

typedef struct tabu_search_solver {
    char *error;
} tabu_search_solver;


void tabu_search_solver_config_init(tabu_search_solver_config *config);
void tabu_search_solver_config_destroy(tabu_search_solver_config *config);

void tabu_search_solver_init(tabu_search_solver *solver);
void tabu_search_solver_destroy(tabu_search_solver *solver);

const char * tabu_search_solver_get_error(tabu_search_solver *solver);

bool tabu_search_solver_solve(tabu_search_solver *solver,
                              tabu_search_solver_config *config,
                              solution *solution);


#endif // TABU_SEARCH_SOLVER_H