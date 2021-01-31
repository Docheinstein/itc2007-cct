#include "hill_climbing.h"
#include "heuristics/neighbourhoods/swap.h"
#include "log/verbose.h"
#include "timeout/timeout.h"
#include "utils/mem_utils.h"

void hill_climbing_params_default(hill_climbing_params *params) {
    params->max_idle = 150000;
}

void hill_climbing(heuristic_solver_state *state, void *arg) {
    hill_climbing_params *params = (hill_climbing_params *) arg;

    long max_idle = params->max_idle >= 0 ? params->max_idle : LONG_MAX;
    int local_best_cost = state->current_cost;
    long idle = 0;
    long iter = 0;

    while (!timeout && idle < max_idle) {
        swap_move swap_mv;
        swap_result swap_result;

        swap_move_generate_random_feasible_effective(state->current_solution, &swap_mv);
        swap_predict(state->current_solution, &swap_mv,
                     NEIGHBOURHOOD_PREDICT_FEASIBILITY_NEVER,
                     NEIGHBOURHOOD_PREDICT_COST_ALWAYS,
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

        if (get_verbosity() >= 2 &&
            idle > 0 && idle % (params->max_idle > 0 ? (params->max_idle / 10) : 10000) == 0)
            verbose2("%s: Iter = %ld | Idle progress = %ld/l%d (%.2f%%) | Current = %d | Local best = %d | Global best = %d",
                     state->methods_name[state->method],
                     iter, idle,
                     params->max_idle > 0 ? params->max_idle : 0,
                     params->max_idle > 0 ? (double) 100 * idle / params->max_idle : 0,
                     state->current_cost, local_best_cost, state->best_cost);
    }
}
