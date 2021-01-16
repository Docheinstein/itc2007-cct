#ifndef LOCAL_SEARCH_SOLVER_H
#define LOCAL_SEARCH_SOLVER_H

#include "solution.h"

typedef struct local_search_solver_config {
    int multistart;
    int time_limit;
    double difficulty_ranking_randomness;
    const solution *starting_solution;
} local_search_solver_config;

typedef struct local_search_solver {
    char *error;
} local_search_solver;


void local_search_solver_config_init(local_search_solver_config *config);
void local_search_solver_config_destroy(local_search_solver_config *config);

void local_search_solver_init(local_search_solver *solver);
void local_search_solver_destroy(local_search_solver *solver);

const char * local_search_solver_get_error(local_search_solver *solver);

bool local_search_solver_solve(local_search_solver *solver,
                               local_search_solver_config *config,
                               solution *solution);

//void local_search(const solution *sol_in, solution *sol_out);

#endif // LOCAL_SEARCH_SOLVER_H