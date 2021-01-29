#include <heuristics/neighbourhoods/swap.h>
#include <math.h>
#include <utils/random_utils.h>
#include "simulated_annealing.h"
#include "log/verbose.h"

#define SA_ACCEPTANCE(delta, t) pow(M_E, - (double) (delta) / (t))


void simulated_annealing_params_default(simulated_annealing_params *params) {
    params->max_idle = 80000;
    params->initial_temperature = 1.5;
    params->cooling_rate = 0.96;
    params->min_temperature = 0.08;
    params->temperature_length_coeff = 1;
}


static bool simulated_annealing_accept(heuristic_solver_state *state,
                                       swap_result *swap_result,
                                       double temperature) {
    if (state->current_cost + swap_result->delta.cost < state->best_cost)
        return true; // always accept best solutions

    double p = SA_ACCEPTANCE(swap_result->delta.cost, temperature);
    double r = rand_uniform(0, 1);
    return r < p;
}

void simulated_annealing(heuristic_solver_state *state, void *arg) {
    MODEL(state->model);
    simulated_annealing_params *params = (simulated_annealing_params *) arg;
    int temperature_length = (int) (state->model->n_lectures * params->temperature_length_coeff);
    verbose("SA.max_idle = %d", params->max_idle);
    verbose("SA.initial_temperature = %g", params->initial_temperature);
    verbose("SA.cooling_rate = %g", params->cooling_rate);
    verbose("SA.min_temperature = %g", params->min_temperature);
    verbose("SA.temperature_length_coeff = %g (=> temperature_length = %d)",
            params->temperature_length_coeff, temperature_length);

    int local_best_cost = state->current_cost;
    int idle = 0;
    int iter = 0;

    double t = params->initial_temperature;

    while (t > params->min_temperature && idle < params->max_idle) {
        if (iter % (temperature_length * 5) == 0)
            verbose2("temperature = %g | p(+1) = %g  p(+5) = %g  p(+10) = %g",
                     t, SA_ACCEPTANCE(1, t), SA_ACCEPTANCE(5, t), SA_ACCEPTANCE(10, t));

        for (int it = 0; it < temperature_length; it++) {
            swap_move swap_mv;
            swap_result swap_result;

            swap_move_generate_random(state->current_solution, &swap_mv, true);
            swap_predict(state->current_solution, &swap_mv,
                         NEIGHBOURHOOD_PREDICT_NEVER,
                         NEIGHBOURHOOD_PREDICT_ALWAYS,
                         &swap_result);

            if (simulated_annealing_accept(state, &swap_result, t)) {
                swap_perform(state->current_solution, &swap_mv,
                             NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
                state->current_cost += swap_result.delta.cost;
                heuristic_solver_state_update(state);
            }

            if (state->current_cost < local_best_cost) {
                local_best_cost = state->current_cost;
                idle = 0;
            }
            else
                idle++;

            if (get_verbosity() >= 2 &&
                    idle > 0 && idle % (params->max_idle / 10) == 0) {
                verbose2("current = %d | local best = %d | global best = %d\n"
                     "iter = %d | idle progress = %d/%d (%g%%)\n"
                     "temperature = %g | p(+1) = %g  p(+5) = %g  p(+10) = %g",
                     state->current_cost, local_best_cost, state->best_cost,
                     iter, idle, params->max_idle, (double) 100 * idle / params->max_idle,
                     t, SA_ACCEPTANCE(1, t), SA_ACCEPTANCE(5, t), SA_ACCEPTANCE(10, t));
            }

            iter++;
        }

        t *= params->cooling_rate;
    }

#undef PRINT_STATS
}
