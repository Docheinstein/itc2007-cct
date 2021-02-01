#include "simulated_annealing.h"
#include <math.h>
#include "heuristics/neighbourhoods/swap.h"
#include "utils/rand_utils.h"
#include "timeout/timeout.h"
#include "log/debug.h"
#include "log/verbose.h"

// p(move) = e^(-delta(move)/temperature)
#define SA_ACCEPTANCE(delta, t) pow(M_E, - (double) (delta) / (t))


void simulated_annealing_params_default(simulated_annealing_params *params) {
    params->max_idle = -1;
    params->initial_temperature = 1.5;
    params->cooling_rate = 0.9995;
    params->min_temperature = 0.10;
    params->temperature_length_coeff = 1;
}

static bool simulated_annealing_accept(heuristic_solver_state *state,
                                       swap_result *swap_result,
                                       double temperature) {
    if (state->current_cost + swap_result->delta.cost < state->best_cost)
        return true; // always accept new best solutions
    return rand_uniform(0, 1) < SA_ACCEPTANCE(swap_result->delta.cost, temperature);
}

void simulated_annealing(heuristic_solver_state *state, void *arg) {
    simulated_annealing_params *params = (simulated_annealing_params *) arg;
    MODEL(state->model);
    bool collect_sa_stats = get_verbosity() >= 2;

    long max_idle = params->max_idle >= 0 ? params->max_idle : LONG_MAX;

    // The default temperature length is equal the number of lectures (~100/500)
    int temperature_length = (int) (state->model->n_lectures * params->temperature_length_coeff);

    int local_best_cost = state->current_cost;
    long idle = 0;
    long iter = 0;
    double t = params->initial_temperature;

    while (!timeout && idle < max_idle && t > params->min_temperature) {
        // Perform temperature_length iters with the same temperature
        for (int it = 0; it < temperature_length; it++) {
            swap_move swap_mv;
            swap_result swap_result;

            swap_move_generate_random_feasible_effective(state->current_solution, &swap_mv);
            swap_predict(state->current_solution, &swap_mv,
                         NEIGHBOURHOOD_PREDICT_FEASIBILITY_NEVER,
                         NEIGHBOURHOOD_PREDICT_COST_ALWAYS,
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

            if (collect_sa_stats &&
                ((idle > 0 && idle % (params->max_idle > 0 ? (params->max_idle / 10) : 10000) == 0)
                || iter % (temperature_length * 5) == 0)) {
                verbose2("%s: Iter = %ld | Idle progress = %ld/%ld (%.2f%%) | "
                         "Current = %d | Local best = %d | Global best = %d | "
                         "Temperature = %.5f | p(+1) = %g  p(+5) = %g  p(+10) = %g",
                     state->methods_name[state->method],
                     iter, idle,
                     params->max_idle > 0 ? params->max_idle : 0,
                     params->max_idle > 0 ? (double) 100 * idle / params->max_idle : 0,
                     state->current_cost, local_best_cost, state->best_cost,
                     t, SA_ACCEPTANCE(1, t), SA_ACCEPTANCE(5, t), SA_ACCEPTANCE(10, t));
            }

            iter++;
        }

        // Decrease the temperature by cooling rate
        t *= params->cooling_rate;
    }
}
