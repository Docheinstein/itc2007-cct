#ifndef HILL_CLIMBING_SOLVER_H
#define HILL_CLIMBING_SOLVER_H

#include <solution.h>

typedef struct hill_climbing_solver_config {
    int idle_limit;
    int time_limit;
    double difficulty_ranking_randomness;
    const solution *starting_solution;
} hill_climbing_solver_config;

typedef struct hill_climbing_solver {
    char *error;
} hill_climbing_solver;

void hill_climbing_solver_config_init(hill_climbing_solver_config *config);
void hill_climbing_solver_config_destroy(hill_climbing_solver_config *config);

void hill_climbing_solver_init(hill_climbing_solver *solver);
void hill_climbing_solver_destroy(hill_climbing_solver *solver);

const char * hill_climbing_solver_get_error(hill_climbing_solver *solver);

bool hill_climbing_solver_solve(hill_climbing_solver *solver,
                               hill_climbing_solver_config *config,
                               solution *solution);

#endif // HILL_CLIMBING_SOLVER_H