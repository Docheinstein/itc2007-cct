#include <log/verbose.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/mem_utils.h>
#include <log/debug.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "hill_climbing.h"

bool hill_climbing_step(heuristic_solver_state *state) {
    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result;

    neighbourhood_swap_generate_random_move(state->current_solution, &swap_mv);
    neighbourhood_swap(state->current_solution, &swap_mv,
                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                       NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PERFORM_NEVER,
                       &swap_result);

    if (swap_result.feasible && swap_result.delta_cost <= 0) {
        neighbourhood_swap(state->current_solution, &swap_mv,
           NEIGHBOURHOOD_PREDICT_NEVER,
           NEIGHBOURHOOD_PREDICT_NEVER,
           NEIGHBOURHOOD_PREDICT_NEVER,
           NEIGHBOURHOOD_PERFORM_ALWAYS,
           NULL);
        state->current_cost += swap_result.delta_cost;
        return heuristic_solver_state_update(state);
    }

    return false;
}

void hill_climbing(heuristic_solver_state *state, void *arg) {
    hill_climbing_params *params = (hill_climbing_params *) arg;
    verbose("HC.max_idle = %d", params->max_idle);

    int idle = 0;
    while (idle < params->max_idle) {
        idle = hill_climbing_step(state) ? 0 : idle + 1;
#if DEBUG2
        if (idle && idle % (max_idle / 10) == 0)
            debug2("Idle progress %d/%d (%g%%)", idle, max_idle,
                  (double) 100 * idle / max_idle);
#endif
    }
}
