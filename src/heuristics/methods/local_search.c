#include <log/verbose.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/mem_utils.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "hill_climbing.h"

void local_search(heuristic_solver_state *state, void *arg) {
    bool improved;

    do {
        improved = false;

        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, state->current_solution);

        neighbourhood_swap_move swap_mv;
        neighbourhood_swap_result swap_result;

        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            neighbourhood_swap(state->current_solution, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.feasible && swap_result.delta_cost < 0) {
                neighbourhood_swap(state->current_solution, &swap_mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   NULL);
                state->current_cost += swap_result.delta_cost;
                heuristic_solver_state_update(state);
                improved = true;
                break;
            }
        }

        neighbourhood_swap_iter_destroy(&swap_iter);
    } while (state->current_cost > 0 && improved);
}