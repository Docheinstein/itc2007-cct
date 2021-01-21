#ifndef ITERATED_LOCAL_SEARCH_SOLVER_H
#define ITERATED_LOCAL_SEARCH_SOLVER_H

#include <solution.h>

typedef struct iterated_local_search_solver_config {
    int idle_limit;
    int time_limit;
    double difficulty_ranking_randomness;
    const solution *starting_solution;
} iterated_local_search_solver_config;

typedef struct iterated_local_search_solver {
    char *error;
} iterated_local_search_solver;

void iterated_local_search_solver_config_init(iterated_local_search_solver_config *config);
void iterated_local_search_solver_config_destroy(iterated_local_search_solver_config *config);

void iterated_local_search_solver_init(iterated_local_search_solver *solver);
void iterated_local_search_solver_destroy(iterated_local_search_solver *solver);

const char * iterated_local_search_solver_get_error(iterated_local_search_solver *solver);

bool iterated_local_search_solver_solve(iterated_local_search_solver *solver,
                               iterated_local_search_solver_config *config,
                               solution *solution);

#endif // ITERATED_LOCAL_SEARCH_SOLVER_H