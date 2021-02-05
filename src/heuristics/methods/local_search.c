#include <heuristics/neighbourhoods/swap.h>
#include <math.h>
#include "local_search.h"
#include "utils/mem_utils.h"
#include "timeout/timeout.h"

void local_search_params_default(local_search_params *params) {
    params->max_distance_from_best_ratio = -1;
}

void local_search(heuristic_solver_state *state, void *arg) {
    local_search_params *params = (local_search_params *) arg;

    if (params->max_distance_from_best_ratio > 0 &&
        state->current_cost > round(state->best_cost * params->max_distance_from_best_ratio)) {
        return; // disabled
    }

    bool improved;

    // Exit conditions: timeout or local minimum reached
    do {
        improved = false;

        swap_iter swap_iter;
        swap_iter_init(&swap_iter, state->current_solution);

        swap_result swap_result;

        while (swap_iter_next(&swap_iter)) {
            swap_predict(state->current_solution, &swap_iter.move,
                         NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE,
                         &swap_result);

            if (swap_result.feasible && swap_result.delta.cost < 0) {
                swap_perform(state->current_solution, &swap_iter.move,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
                state->current_cost += swap_result.delta.cost;
                heuristic_solver_state_update(state);
                improved = true;
                break;
            }
        }

        swap_iter_destroy(&swap_iter);
    } while(!timeout && improved);
}
