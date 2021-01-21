#ifndef SIMULATED_ANNEALING_SOLVER_H
#define SIMULATED_ANNEALING_SOLVER_H

#include <solution.h>

typedef struct simulated_annealing_solver_config {
    int idle_limit;
    int time_limit;
    double difficulty_ranking_randomness;
    const solution *starting_solution;
} simulated_annealing_solver_config;

typedef struct simulated_annealing_solver {
    char *error;
} simulated_annealing_solver;

void simulated_annealing_solver_config_init(simulated_annealing_solver_config *config);
void simulated_annealing_solver_config_destroy(simulated_annealing_solver_config *config);

void simulated_annealing_solver_init(simulated_annealing_solver *solver);
void simulated_annealing_solver_destroy(simulated_annealing_solver *solver);

const char * simulated_annealing_solver_get_error(simulated_annealing_solver *solver);

bool simulated_annealing_solver_solve(simulated_annealing_solver *solver,
                               simulated_annealing_solver_config *config,
                               solution *solution);

#endif // SIMULATED_ANNEALING_SOLVER_H