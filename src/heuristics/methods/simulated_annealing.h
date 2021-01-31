#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "heuristics/heuristic_solver.h"

/*
 * Simulated Annealing.
 * Generates a random move and performs it with a probability:
 * p(move) = e^(-delta(move)/temperature).
 * Where delta(move) is the cost introduced by performing move (negative is better).
 * Performs `temperature_length` iteration with the same temperature and then
 * decrease it by `cooling_rate` until it reaches `min_temperature`
 *
 * `max_idle` defines after how many iterations of non-decreasing
 *      cost (side moves) the method must quit.
 * `initial_temperature` defines the initial temperature of the SA schedule.
 * `cooling_rate` defines the rate at which decrease the temperature:
 *      after `temperature_length`: temperature = temperature * cooling_rate
 * `min_temperature` defines the temperature to reach before
 *      the method must quit
 * `temperature_length_coeff`: multiply the default temperature length
 *      (number of lectures of the model) by `temperature_length_coeff`
 */

typedef struct simulated_annealing_params {
    long max_idle;
    double initial_temperature;
    double cooling_rate;
    double min_temperature;
    double temperature_length_coeff;
} simulated_annealing_params;

void simulated_annealing_params_default(simulated_annealing_params *params);

void simulated_annealing(heuristic_solver_state *state, void *arg);

#endif // SIMULATED_ANNEALING_H