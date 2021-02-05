#include "tabu_search.h"
#include "heuristics/neighbourhoods/swap.h"
#include "log/verbose.h"
#include "timeout/timeout.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "utils/rand_utils.h"


void tabu_search_params_default(tabu_search_params *params) {
    params->max_idle = -1;
    params->max_idle_near_best_coeff = 1.5;
    params->near_best_ratio = 1.02;
    params->tabu_tenure = 120;
    params->frequency_penalty_coeff = 0;
}

typedef struct tabu_list_entry {
    long time; // iteration of the last ban
    int count; // frequency of the ban
} tabu_list_entry;

typedef struct tabu_list {
    const model *model;
    tabu_list_entry *banned; // tabu list: implemented as a matrix of `tabu_list_entry`
    int tenure;
    double frequency_penalty_coeff;
} tabu_list;

static void tabu_list_init(tabu_list *tabu, const model *m,
                           int tenure, double frequency_penalty_coeff) {
    MODEL(m);
    tabu->model = model;
    tabu->banned = callocx(C * R * D * S, sizeof(tabu_list_entry));
    tabu->tenure = tenure;
    tabu->frequency_penalty_coeff = frequency_penalty_coeff;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->banned);
}

static bool tabu_list_lecture_is_allowed(tabu_list *tabu, int c, int r, int d, int s, long time) {
    if (c < 0)
        return true;
    MODEL(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
    if (!entry->count)
        return true; // allowed: not in the tabu list

    // The move is allowed after `tt + coeff * count(move)` iteration
    return entry->time + tabu->tenure + (long) (tabu->frequency_penalty_coeff * entry->count) < time;
}

static bool tabu_list_move_is_allowed(tabu_list *tabu, const swap_move *mv, long time) {
    /*
     * A move is allowed if the course c1 does not came back to (r2, d2, s2)
     * and if the course c2 does not came back to (r1, d1, s1) within tt iterations.
     */
    return
        tabu_list_lecture_is_allowed(tabu, mv->helper.c1, mv->r2, mv->d2, mv->s2, time) &&
        tabu_list_lecture_is_allowed(tabu, mv->helper.c2, mv->helper.r1, mv->helper.d1, mv->helper.s1, time);
}

static void tabu_list_ban_assignment(tabu_list *tabu, int c, int r, int d, int s, long time) {
    if (c < 0)
        return;
    MODEL(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
    entry->time = time;
    entry->count++;
}

static void tabu_list_clear(tabu_list *tabu) {
    MODEL(tabu->model);
    memset(tabu->banned, 0, C * R * D * S * sizeof(tabu_list_entry));
}

static void tabu_list_ban_move(tabu_list *tabu, swap_move *mv, long time) {
    tabu_list_ban_assignment(tabu, mv->helper.c1, mv->helper.r1, mv->helper.d1, mv->helper.s1, time);
    tabu_list_ban_assignment(tabu, mv->helper.c2, mv->r2, mv->d2, mv->s2, time);
}

void tabu_search(heuristic_solver_state *state, void *arg) {
    tabu_search_params *params = (tabu_search_params *) arg;
    MODEL(state->model);
    bool ts_stats = get_verbosity() >= 2;

    long max_idle = params->max_idle >= 0 ? params->max_idle : LONG_MAX;
    long max_idle_near_best = (long) (params->max_idle_near_best_coeff * (double) params->max_idle);
    max_idle_near_best = max_idle_near_best >= 0 ? max_idle_near_best : LONG_MAX;
    int local_best_cost = state->current_cost;
    long idle = 0;
    long iter = 0;

    tabu_list tabu;
    tabu_list_init(&tabu, state->model,
                   params->tabu_tenure, params->frequency_penalty_coeff);

    swap_move *moves = mallocx(tabu.model->n_lectures * R * D * S, sizeof(swap_move));
    struct {
        int n_banned_moves;
        int n_side_moves;
        int n_side_banned_moves;
    } stats = {0, 0, 0};

    // Exit conditions: timeout or exceed max_idle (eventually increased if near best)
    while (!timeout &&
            ((state->current_cost < params->near_best_ratio * state->best_cost) ?
                idle <= max_idle_near_best : idle <= max_idle)) {
        swap_iter swap_iter;
        swap_iter_init(&swap_iter, state->current_solution);

        swap_result swap_result;

        int move_cursor = 0;
        if (ts_stats)
            stats.n_side_moves = stats.n_banned_moves = stats.n_side_banned_moves = 0;

        int best_swap_cost = INT_MAX;

        while (swap_iter_next(&swap_iter)) {
            swap_predict(state->current_solution, &swap_iter.move,
                         NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE,
                         &swap_result);

            if (!swap_result.feasible)
                continue;

            if (ts_stats) {
                bool banned = !tabu_list_move_is_allowed(&tabu, &swap_iter.move, iter);
                bool side = swap_result.delta.cost == 0;
                stats.n_banned_moves += banned;
                stats.n_side_moves += side;
                stats.n_side_banned_moves += (banned && side);
            }

            if (swap_result.delta.cost <= best_swap_cost &&
                // Accept only if does not belong to the tabu banned
                (tabu_list_move_is_allowed(&tabu, &swap_iter.move, iter) ||
                 // exception: aspiration criteria
                 state->current_cost + swap_result.delta.cost < state->best_cost)) {

                if (swap_result.delta.cost < best_swap_cost) {
                    move_cursor = 0; // "clear" the best moves array
                    best_swap_cost = swap_result.delta.cost;
                }

                moves[move_cursor++] = swap_iter.move;
            }
        }

        swap_iter_destroy(&swap_iter);

        if (best_swap_cost != INT_MAX) {
            // Pick a random move among the best ones
            swap_move *mv = &moves[rand_range(0, move_cursor)];
            swap_perform(state->current_solution, mv,
                         NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);

            state->current_cost += best_swap_cost;
            heuristic_solver_state_update(state);
            tabu_list_ban_move(&tabu, mv, iter);
        }

        if (state->current_cost < local_best_cost) {
            local_best_cost = state->current_cost;
            idle = 0;
        }
        else {
            idle++;
        }

        if (ts_stats &&
            (idle > 0 && idle % (params->max_idle >= 10 ? (params->max_idle / 10) : 100) == 0)) {
            verbose2("%s: Iter = %ld | Idle progress = %ld/%ld (%.2f%%) | "
                     "Current = %d | Local best = %d | Global best = %d | "
                     "# Banned = %d/%d | # Side = %d/%d (%d/%d banned) | "
                     "# Elite = %d (cost = %d)",
                     state->methods_name[state->method],
                     iter, idle,
                     params->max_idle > 0 ? params->max_idle : 0,
                     params->max_idle > 0 ? (double) 100 * idle / params->max_idle : 0,
                     state->current_cost, local_best_cost, state->best_cost,
                     stats.n_banned_moves, swap_iter.i, stats.n_side_moves, swap_iter.i,
                     stats.n_side_banned_moves, stats.n_side_moves, move_cursor, best_swap_cost);
        }

        iter++;
    }

    tabu_list_destroy(&tabu);
    free(moves);
}
