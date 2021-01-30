#include <log/verbose.h>
#include <utils/mem_utils.h>
#include "heuristics/neighbourhoods/swap.h"
#include "hill_climbing.h"
#include <log/debug.h>

void hill_climbing_params_default(hill_climbing_params *params) {
    params->max_idle = 80000;
    params->intensification_threshold = 1.1;
    params->intensification_coeff = 1.5;
}

void hill_climbing(heuristic_solver_state *state, void *arg) {
    hill_climbing_params *params = (hill_climbing_params *) arg;
    debug("HC.max_idle = %d", params->max_idle);
    debug("HC.intensification_threshold = %f", params->intensification_threshold);
    debug("HC.intensification_coeff = %f", params->intensification_coeff);

    int local_best_cost = state->current_cost;
    int idle = 0;
    int iter = 0;

    while (idle < params->max_idle ||
                (state->current_cost < params->intensification_threshold * state->best_cost &&
                idle < params->max_idle * params->intensification_coeff)) {
        int prev_cost = state->current_cost;

        swap_move swap_mv;
        swap_result swap_result;

        swap_move_generate_random(state->current_solution, &swap_mv, true);
        swap_predict(state->current_solution, &swap_mv,
                     NEIGHBOURHOOD_PREDICT_NEVER,
                     NEIGHBOURHOOD_PREDICT_ALWAYS,
                     &swap_result);

        if (swap_result.delta.cost <= 0) {
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

        if (idle && idle % (params->max_idle / 10) == 0)
            verbose2("%s: Iter = %d | Idle progress = %d/%d (%.2f%%) | Current = %d | Local best = %d | Global best = %d",
                     state->methods_name[state->method],
                     iter, idle, params->max_idle, (double) 100 * idle / params->max_idle,
                     state->current_cost, local_best_cost, state->best_cost);
    }
}
