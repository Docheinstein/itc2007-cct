#include <heuristics/neighbourhoods/swap.h>
#include <math.h>
#include <log/debug.h>
#include <log/verbose.h>
#include "deep_local_search.h"
#include "utils/mem_utils.h"
#include "timeout/timeout.h"

void deep_local_search_params_default(deep_local_search_params *params) {
    params->max_distance_from_best_ratio = -1;
}

typedef struct swap_move_result {
    swap_move move;
    int delta;
} swap_move_result;

static int int_compare(const void *_1, const void *_2) {
    int *i1 = (int *) _1;
    int *i2 = (int *) _2;
    return *i2 - *i1;
}
void deep_local_search(heuristic_solver_state *state, void *arg) {
    deep_local_search_params *params = (deep_local_search_params *) arg;

    if (params->max_distance_from_best_ratio > 0 &&
        state->current_cost > round(state->best_cost * params->max_distance_from_best_ratio)) {
        return; // disabled
    }


    int diving = 0;

    swap_move_result *moves = mallocx(swap_neighbourhood_maximum_size(state->current_solution->model),
                                      sizeof(swap_move_result));
    int move_cursor;

    // Exit conditions: timeout or local minimum reached
    bool improved;
    do {
        improved = false;
        move_cursor = 0;

        swap_iter swap_iter;
        swap_iter_init(&swap_iter, state->current_solution);

        swap_result swap_result;

        // First of all sort all the moves at distance 1 by
        // decreasing cost, since the moves with better cost are more
        // likely to lead to a move pair with negative cost.
        // If a move with cost < 0 is found while iterating, perform
        // the move immediately (as local_search does).

        bool performed_deep1 = false;
        while (swap_iter_next(&swap_iter)) {
            swap_predict(state->current_solution, &swap_iter.move,
                         NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE,
                         &swap_result);

            if (!swap_result.feasible)
                continue;

            if (swap_result.delta.cost < 0) {
                // Perform improving move immediately
                swap_perform(state->current_solution, &swap_iter.move,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
                state->current_cost += swap_result.delta.cost;
                heuristic_solver_state_update(state);
                performed_deep1 = true;
                break;
            } else {
                // Push to the moves array
                moves[move_cursor].move = swap_iter.move;
                moves[move_cursor].delta = swap_result.delta.cost;
                move_cursor++;
            }
        }
        swap_iter_destroy(&swap_iter);

        if (performed_deep1) {
            // Already did a move, restart
            improved = true;
            continue;
        }

        verbose2("No improving move in neighbourhood at distance 1, going deeper...");
        qsort(moves, move_cursor, sizeof(swap_move_result), int_compare);

        // Look at the neighbourhood of each move
        bool performed_deep_2 = false;

        for (int i = 0; i < move_cursor; i++) {
            const swap_move *mv1 = &moves[i].move;
            int mv1_cost = moves[i].delta;
            debug("[%d/%d] Inspecting move %d %d %d %d of cost %d",
                  i, move_cursor, mv1->l1, mv1->r2, mv1->d2, mv1->s2, mv1_cost);

            if (get_verbosity() >= 2 && i > 0 && i % (move_cursor / 50) == 0) {
                verbose2("%s: Diving = %d | Diving progress = %d/%d (%.2f%%) | Current = %d | Global best = %d",
                         state->methods_name[state->method],
                         diving, i, move_cursor, (double) 100 * i / move_cursor,
                         state->current_cost, state->best_cost);
            }

            // We have to perform the move to look at the neighbourhood.
            // If every move of the neighbourhood of this neighbourhood
            // does not lead to a pair of move with delta < 0, we'll do
            // the reverse move to going back.
            swap_perform(state->current_solution, mv1,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);

            swap_iter_init(&swap_iter, state->current_solution);

            while (swap_iter_next(&swap_iter)) {
                swap_predict(state->current_solution, &swap_iter.move,
                             NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                             NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE,
                             &swap_result);

                if (!swap_result.feasible)
                    continue;

                if (swap_result.delta.cost + mv1_cost < 0) {
                    verbose("%s: Diving = %d | Performing pair of moves with negative delta = %d (move 1 = %d, move 2 = %d)",
                            state->methods_name[state->method],
                            diving,
                            mv1_cost + swap_result.delta.cost, mv1_cost, swap_result.delta.cost);
                    swap_perform(state->current_solution, &swap_iter.move,
                                NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
                    state->current_cost += swap_result.delta.cost + mv1_cost;
                    heuristic_solver_state_update(state);
                    performed_deep_2 = true;
                    break;
                }
            }
            swap_iter_destroy(&swap_iter);

            if (performed_deep_2) {
                improved = true;
                break;
            }

            // Otherwise, no good pair of move -> going back
            swap_move mv1_back;
            swap_move_reverse(mv1, &mv1_back);

            swap_perform(state->current_solution, &mv1_back,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
        }

        diving++;
    } while(!timeout && improved);
}
