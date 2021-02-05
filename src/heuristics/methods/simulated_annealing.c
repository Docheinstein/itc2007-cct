#include "simulated_annealing.h"
#include <math.h>
#include "heuristics/neighbourhoods/swap.h"
#include "utils/rand_utils.h"
#include "timeout/timeout.h"
#include "log/verbose.h"

// p(move) = e^(-delta(move)/temperature)
#define SA_ACCEPTANCE(delta, t) pow(M_E, - (double) (delta) / (t))


void simulated_annealing_params_default(simulated_annealing_params *params) {
    params->initial_temperature = 1.4;
//    params->initial_temperature = 1.3;
//    params->cooling_rate = 0.965;
//    params->temperature_length_coeff = 50;
    params->cooling_rate = 0.965;
    params->temperature_length_coeff = 0.125;
//    params->temperature_length_coeff = 0.2;
//    params->min_temperature = 0.11;
    params->min_temperature = 0.12;
//    params->min_temperature_near_best_coeff = 0.72;
    params->min_temperature_near_best_coeff = 0.68;
    params->near_best_ratio = 1.05;
    params->reheat_coeff = 1.015;
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
    bool sa_stats = get_verbosity() >= 2;

    // The default temperature length is equal the number of lectures (~100/500)
//    int temperature_length = (int) (state->model->n_lectures * params->temperature_length_coeff);
    int temperature_length = (int) (L * R * D * S * params->temperature_length_coeff);
    int temperature_length_5_times = 5 * temperature_length;
    double t = params->initial_temperature;
    double t_min = params->min_temperature;
    double cooling_rate = params->cooling_rate;
    double t_min_near_best = t_min * params->min_temperature_near_best_coeff;

    double reheat = pow(params->reheat_coeff, (double) state->non_improving_best_cycles);
    t *= reheat;

    int local_best_cost = state->current_cost;
    long idle = 0;
    long iter = 0;


    // Exit conditions: timeout or below minimum temperature
    while (!timeout &&
            ((state->current_cost < round(params->near_best_ratio * state->best_cost)) ?
                t > t_min_near_best : t > t_min)) {
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

            if (sa_stats &&
                ((idle > 0 && idle % 10000) == 0)
                || iter > 0 && iter % (temperature_length_5_times) == 0) {
                verbose2("%s: Iter = %ld | Idle = %ld | "
                         "Current = %d | Local best = %d | Global best = %d | "
                         "Temperature = %.5f | p(+1) = %g  p(+5) = %g  p(+10) = %g | "
                         "T_length = %d | Cooling Rate = %.5f",
                     state->methods_name[state->method],
                     iter, idle,
                     state->current_cost, local_best_cost, state->best_cost,
                     t, SA_ACCEPTANCE(1, t), SA_ACCEPTANCE(5, t), SA_ACCEPTANCE(10, t),
                     temperature_length, cooling_rate);
            }

            iter++;
        }

        // Decrease the temperature by cooling rate
        t *= cooling_rate;
    }
}
