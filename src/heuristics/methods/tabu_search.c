#include <heuristics/neighbourhoods/neighbourhood_swap.h>
#include <renderer.h>
#include <utils/random_utils.h>
#include <utils/def_utils.h>
#include "tabu_search.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "math.h"
#include "log/debug.h"
#include "log/verbose.h"

typedef struct tabu_list_entry {
    int time;
    int count;
    int delta_cost_sum;
} tabu_list_entry;

typedef struct tabu_list {
    const model *model;
    tabu_list_entry *banned;
    int tenure;
    double frequency_penalty_coeff;
} tabu_list;


static void tabu_list_init(tabu_list *tabu, const model *model,
                           int tenure, double frequency_penalty_coeff) {
    CRDSQT(model);
    tabu->model = model;
    tabu->banned = callocx(C * R * D * S, sizeof(tabu_list_entry));
    tabu->tenure = tenure;
    tabu->frequency_penalty_coeff = frequency_penalty_coeff;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->banned);
}

static bool tabu_list_lecture_is_allowed(tabu_list *tabu, int c, int r, int d, int s, int time) {
    if (c < 0)
        return true;
    CRDSQT(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
    if (!entry->count)
        return true;
    return entry->time + tabu->tenure * pow(tabu->frequency_penalty_coeff, entry->count) < time;
//    return entry->time + tabu->tenure + tabu->frequency_penalty_coeff * entry->count < time;
}

static bool tabu_list_move_is_allowed(tabu_list *tabu, neighbourhood_swap_move *mv, int time) {
    return
        tabu_list_lecture_is_allowed(tabu, mv->c1, mv->r2, mv->d2, mv->s2, time) &&
        tabu_list_lecture_is_allowed(tabu, mv->c2, mv->r1, mv->d1, mv->s1, time);
}

static void tabu_list_insert_lecture(tabu_list *tabu, int c, int r, int d, int s,
                                     int time) {
    if (c < 0)
        return;
    CRDSQT(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
    entry->time = time;
    entry->count++;
}

static void tabu_list_clear(tabu_list *tabu) {
    CRDSQT(tabu->model);
    memset(tabu->banned, 0, C * R * D * S * sizeof(tabu_list_entry));
}

static void tabu_list_insert_move(tabu_list *tabu, neighbourhood_swap_move *mv,
                                  int time) {
    tabu_list_insert_lecture(tabu, mv->c1, mv->r1, mv->d1, mv->s1, time);
    tabu_list_insert_lecture(tabu, mv->c2, mv->r2, mv->d2, mv->s2, time);
}

static void tabu_list_dump(tabu_list *tabu) {
#if DEBUG2
    CRDSQT(tabu->model);
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
                    debug2("Tabu[%d][%d][%d][%d] = {time=%d, count=%d, delta_sum=%d (mean=%g)}",
                          c, r, d, s, entry->time, entry->count, entry->delta_cost_sum, (double) entry->delta_cost_sum / entry->count);
                }
            }
        }
    };
#endif
}

static void apply_solution_fingerprint_diff(solution_fingerprint_t *f,
                                     solution_fingerprint_t diff_plus,
                                     solution_fingerprint_t diff_minus) {
    f->sum += diff_plus.sum;
    f->sum -= diff_minus.sum;
    f->xor ^= diff_plus.xor;
    f->xor ^= diff_minus.xor;
}

void tabu_search(heuristic_solver_state *state, void *arg) {
    CRDSQT(state->model);

    tabu_search_params *params = (tabu_search_params *) arg;
    verbose("TS.max_idle = %d", params->max_idle);
    verbose("TS.tabu_tenure = %d", params->tabu_tenure);
    verbose("TS.frequency_penalty_coeff = %g", params->frequency_penalty_coeff);
    verbose("TS.random_pick = %s", BOOL_TO_STR(params->random_pick));
    verbose("TS.clear_on_best = %s", BOOL_TO_STR(params->clear_on_best));

    tabu_list tabu;
    tabu_list_init(&tabu, state->model,
                   params->tabu_tenure, params->frequency_penalty_coeff);

    int local_best_cost = state->current_cost;

    int idle = 0;
    int iter = 0;

    neighbourhood_swap_move *moves = mallocx(
            params->random_pick ? (tabu.model->n_lectures * R * D * S) : 1,
            sizeof(neighbourhood_swap_move));

    while (idle < params->max_idle) {
        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, state->current_solution);

        neighbourhood_swap_move swap_mv;
        neighbourhood_swap_result swap_result;

        int best_swap_cost = INT_MAX;

        struct {
            int n_banned_moves;
            int n_side_moves;
            int n_side_banned_moves;
        } stats = {0, 0, 0};

        int move_cursor = 0;

        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            neighbourhood_swap(state->current_solution, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.feasible &&
                    swap_result.delta_cost <= best_swap_cost &&
                    (state->current_cost + swap_result.delta_cost < state->best_cost ||
                            tabu_list_move_is_allowed(&tabu, &swap_mv, iter))) {

                if (swap_result.delta_cost < best_swap_cost) {
                    move_cursor = 0;
                    best_swap_cost = swap_result.delta_cost;
                }

                moves[move_cursor] = swap_mv;

                if (params->random_pick)
                    move_cursor++;

                if (swap_result.delta_cost < 0 && params->steepest) {
                    // Go down without evaluating all the neighbours
                    break;
                }
            }

            bool banned = !tabu_list_move_is_allowed(&tabu, &swap_mv, iter);
            bool side = swap_result.delta_cost == 0;
            stats.n_banned_moves +=  banned;
            stats.n_side_moves += side;
            stats.n_side_banned_moves += (banned && side);
        }

        neighbourhood_swap_iter_destroy(&swap_iter);

        if (best_swap_cost != INT_MAX) {
            int pick = params->random_pick ? rand_int_range(0, move_cursor) : 0;
            swap_mv = moves[pick];
            neighbourhood_swap(state->current_solution, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);

            state->current_cost += swap_result.delta_cost;
            bool new_best = heuristic_solver_state_update(state);
            if (new_best && params->clear_on_best)
                tabu_list_clear(&tabu);
            tabu_list_insert_move(&tabu, &swap_mv, iter);
        }


        if (state->current_cost < local_best_cost) {
            local_best_cost = state->current_cost;
            idle = 0;
        }
        else
            idle++;

        if (get_verbosity() >= 2 &&
            (idle > 0 && idle % (params->max_idle / 10) == 0)) {
            verbose2("current = %d | local best = %d | global best = %d\n"
                     "iter = %d | idle progress = %d/%d (%g%%)\n"
                     "# banned = %d/%d | # side = %d/%d (%d/%d banned)",
                     state->current_cost, local_best_cost, state->best_cost,
                     iter, idle, params->max_idle, (double) 100 * idle / params->max_idle,
                     stats.n_banned_moves, swap_iter.i, stats.n_side_moves, swap_iter.i,
                     stats.n_side_banned_moves, stats.n_side_moves);
        }

        iter++;
    }

    tabu_list_destroy(&tabu);
    free(moves);
}
