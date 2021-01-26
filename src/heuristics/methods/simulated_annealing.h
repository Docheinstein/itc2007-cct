#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "heuristics/heuristic_solver.h"

typedef struct simulated_annealing_params {
    int max_idle;
    double temperature;
    double min_temperature;
    double temperature_length_coeff;
    double cooling_rate;
} simulated_annealing_params;

void simulated_annealing(heuristic_solver_state *state, void *arg);

#endif // SIMULATED_ANNEALING_H