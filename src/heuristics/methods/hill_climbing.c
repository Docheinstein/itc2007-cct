#include <log/verbose.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/mem_utils.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "hill_climbing.h"
#include <log/debug.h>

void hill_climbing(heuristic_solver_state *state, void *arg) {
    hill_climbing_params *params = (hill_climbing_params *) arg;
    verbose("HC.max_idle = %d", params->max_idle);

    int local_best_cost = state->current_cost;
    int idle = 0;
    int iter = 0;

    while (idle < params->max_idle) {
        int prev_cost = state->current_cost;

        neighbourhood_swap_move swap_mv;
        neighbourhood_swap_result swap_result;

        neighbourhood_swap_generate_random_feasible_move(state->current_solution, &swap_mv);
        neighbourhood_swap(state->current_solution, &swap_mv,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PERFORM_NEVER,
                           &swap_result);

        if (swap_result.delta_cost <= 0) {
            neighbourhood_swap(state->current_solution, &swap_mv,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);
            state->current_cost += swap_result.delta_cost;
            heuristic_solver_state_update(state);
        }

        if (state->current_cost < prev_cost)
            idle = 0;
        else
            idle++;

        if (idle && idle % (params->max_idle / 10) == 0)
            verbose2("current = %d | local best = %d | global best = %d\n"
                     "iter = %d | idle progress = %d/%d (%g%%)",
                     state->current_cost, local_best_cost, state->best_cost,
                     iter, idle, params->max_idle, (double) 100 * idle / params->max_idle);
    }
}
