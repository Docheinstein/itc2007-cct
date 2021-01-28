#include <log/verbose.h>
#include <utils/mem_utils.h>
#include <heuristics/neighbourhoods/swap.h>
#include "hill_climbing.h"
#include "local_search.h"

void local_search(heuristic_solver_state *state, void *arg) {
    local_search_params *params = (local_search_params *) arg;

    bool improved;

    do {
        improved = false;

        swap_iter swap_iter;
        swap_iter_init(&swap_iter, state->current_solution);

        swap_move swap_mv;
        swap_result swap_result;

        swap_move best_swap_mv;
        int best_swap_cost = INT_MAX;

        while (swap_iter_next(&swap_iter, &swap_mv)) {
            swap_predict(state->current_solution, &swap_mv,
                         NEIGHBOURHOOD_PREDICT_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                         &swap_result);

            if (swap_result.feasible && swap_result.delta.cost < best_swap_cost) {
                best_swap_cost = swap_result.delta.cost;
                best_swap_mv = swap_mv;
                if (swap_result.delta.cost < 0 && params->steepest) {
                    // Perform an improving move without evaluating
                    // all the neighbours
                    break;
                }
            }
        }

        swap_iter_destroy(&swap_iter);

        if (best_swap_cost < 0) {
            swap_perform(state->current_solution, &best_swap_mv,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
            state->current_cost += best_swap_cost;
            heuristic_solver_state_update(state);
            improved = true;
        }

    } while (improved);
}