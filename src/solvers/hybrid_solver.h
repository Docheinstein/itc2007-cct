#ifndef HYBRID_SOLVER_H
#define HYBRID_SOLVER_H

#include "solution.h"

typedef struct hybrid_solver_config {
    int multistart;
    int time_limit;
    double difficulty_ranking_randomness;
    const solution *starting_solution;
} hybrid_solver_config;

typedef struct hybrid_solver {
    char *error;
} hybrid_solver;

void hybrid_solver_config_init(hybrid_solver_config *config);
void hybrid_solver_config_destroy(hybrid_solver_config *config);

void hybrid_solver_init(hybrid_solver *solver);
void hybrid_solver_destroy(hybrid_solver *solver);

const char * hybrid_solver_get_error(hybrid_solver *solver);

bool hybrid_solver_solve(hybrid_solver *solver,
                               hybrid_solver_config *config,
                               solution *solution);

#endif // HYBRID_SOLVER_H