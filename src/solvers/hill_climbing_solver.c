#include "hill_climbing_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/assert_utils.h>
#include <utils/mem_utils.h>
#include <utils/random_utils.h>
#include <math.h>
#include <utils/math_utils.h>
#include <utils/array_utils.h>
#include <utils/def_utils.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

/*
        // Phase 2
#define PHASE2 1
#if PHASE2
        debug("[%d] Phase 2: hill_climbing_local_search_phase", state.cycle);
        hill_climbing_local_search_phase(&state,
                                         neighbourhood_swap_accept_less_cost, NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_step] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif

        // Phase 3
#define PHASE3 0
#if PHASE3
        debug("[%d] Phase 3: hill_climbing_local_search_depth_2_phase", state.cycle);
        hill_climbing_local_search_depth_2_phase(&state,
                                                 neighbourhood_swap_accept_less_or_equal_0,
                                                 neighbourhood_swap_accept2_less_or_equal_cost,
                                                 NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_depth_2_phase] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif

#define PHASE4 0
#if PHASE4
        const double MAX_TOL = .10;
        double tolerance = 1 + MAX_TOL - map((double) starting_time, (double) time_limit, 0, MAX_TOL, (double) ms());
//        debug("tol =%g", tolerance);
        hill_climbing_phase(s, &cost, best_solution_cost, neighbourhood_swap_accept_cost_tolerance, &tolerance, 100);
        debug("[%d] Cost after phase 2: %d      // best is %d", major_cycle, cost, best_solution_cost);

        if (cost < best_solution_cost) {
            verbose("[%d] New best solution found, cost = %d", major_cycle, cost);
            best_solution_cost = cost;
            solution_copy(&best_solution, s);
        }
#endif

        // Phase 1
#define PHASE5 0
#if PHASE5
        debug("[%d] Phase 3: hill_climbing_local_search_depth_2_phase", state.cycle);
        hill_climbing_local_search_depth_2_phase(&state,
                                                 neighbourhood_swap_accept_less_or_equal_0,
                                                 neighbourhood_swap_accept2_less_or_equal_cost,
                                                 NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_depth_2_phase] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif
*/

void hill_climbing_solver_config_init(hill_climbing_solver_config *config) {
    config->idle_limit = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void hill_climbing_solver_config_destroy(hill_climbing_solver_config *config) {

}

void hill_climbing_solver_init(hill_climbing_solver *solver) {
    solver->error = NULL;
}

void hill_climbing_solver_destroy(hill_climbing_solver *solver) {
    free(solver->error);
}

const char *hill_climbing_solver_get_error(hill_climbing_solver *solver) {
    return solver->error;
}

void hill_climbing_solver_reinit(hill_climbing_solver *solver) {
    hill_climbing_solver_destroy(solver);
    hill_climbing_solver_init(solver);
}

typedef struct tabu_list_entry {
    int time;
    int count;
    int delta_sum;
} tabu_list_entry;

typedef struct tabu_list {
    const model *model;
//    tabu_list_entry *banned;
//    GHashTable *banned;
    tabu_list_entry *banned;
    int tenure;
//    int cursor;
} tabu_list;

//
//typedef struct tabu_list_entry_key {
//    tabu_list *tabu;
//    int c, r, d, s;
//} tabu_list_entry_key;
//
//typedef struct tabu_list_entry_value {
//    int time;
//} tabu_list_entry_value;
//
//static guint tabu_list_entry_key_hash(gconstpointer  key) {
//    const tabu_list_entry_key *stat_key = (tabu_list_entry_key *) key;
//    CRDSQT(stat_key->tabu->model);
//    return INDEX4(stat_key->c, C, stat_key->r, R, stat_key->d, D, stat_key->s, S);
//}
//
//static gboolean tabu_list_entry_key_equal(gconstpointer  a, gconstpointer  b) {
//    const tabu_list_entry_key *m1 = (tabu_list_entry_key *) a;
//    const tabu_list_entry_key *m2 = (tabu_list_entry_key *) b;
//    return
//        m1->c == m2->c &&
//        m1->r == m2->r &&
//        m1->d == m2->d &&
//        m1->s == m2->s;
//}

void tabu_list_init(tabu_list *tabu, const model *model, int initial_size) {
//    tabu->banned = mallocx(size, sizeof(tabu_list_entry));
//    memset(tabu->banned, -1, size * sizeof(tabu_list_entry));
    CRDSQT(model);
    tabu->model = model;
    tabu->banned = callocx(C * R * D * S, sizeof(tabu_list_entry));
    tabu->tenure = initial_size;
//        tabu->banned = g_hash_table_new_full(
//            tabu_list_entry_key_hash, tabu_list_entry_key_equal, free, free);
//    tabu->len = size;
//    tabu->cursor = 0;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->banned);
//    g_hash_table_destroy(tabu->banned);
}

double tabu_list_move_penalty(tabu_list *tabu, neighbourhood_swap_move *mv) {
    static const double TT_COUNT_MIN_PENALIZER = 0.96;
    static const double TT_COUNT_MID_PENALIZER = 1.05;
    static const double TT_COUNT_MAX_PENALIZER = 1.12;

    CRDSQT(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(mv->c1, C, mv->r2, R, mv->d2, D, mv->s2, S)];
    if (!entry->count)
        return 1;
    double delta_mean = (double ) entry->delta_sum / entry->count;
    double penalizer;
    if (delta_mean < 0)
        penalizer = map(0, 1, TT_COUNT_MIN_PENALIZER, TT_COUNT_MID_PENALIZER, rand_uniform(0, 1));
    else
        penalizer = map(0, 1, TT_COUNT_MID_PENALIZER, TT_COUNT_MAX_PENALIZER, rand_uniform(0, 1));
    return penalizer;
//    return 1;

}

bool tabu_list_is_allowed(tabu_list *tabu, neighbourhood_swap_move *mv, int time) {
    CRDSQT(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(mv->c1, C, mv->r2, R, mv->d2, D, mv->s2, S)];
    return entry->time + tabu->tenure * pow(tabu_list_move_penalty(tabu, mv), entry->count) < time;
}

void tabu_list_insert(tabu_list *tabu, neighbourhood_swap_move *mv, neighbourhood_swap_result *res, int time, int assert_) {
    CRDSQT(tabu->model);
    tabu_list_entry *entry = &tabu->banned[INDEX4(mv->c1, C, mv->r1, R, mv->d1, D, mv->s1, S)];
//    if (assert_)
//        assert(tabu_list_is_allowed(tabu, mv, time));
    entry->time = time;
    entry->count++;
    entry->delta_sum += res->delta_cost;
}

void tabu_list_dump(tabu_list *tabu) {
    CRDSQT(tabu->model);
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    tabu_list_entry *entry = &tabu->banned[INDEX4(c, C, r, R, d, D, s, S)];
                    debug("Tabu[%d][%d][%d][%d] = {time=%d, count=%d, delta_sum=%d (mean=%g)}",
                          c, r, d, s, entry->time, entry->count, entry->delta_sum, (double) entry->delta_sum / entry->count);
                }
            }
        }
    };
}
//
//bool tabu_list_is_allowed(tabu_list *tabu, neighbourhood_swap_move *mv, int time) {
//    tabu_list_entry_key key = {
//        .tabu = tabu,
//        .c = mv->c1,
//        .r = mv->r2,
//        .d = mv->d2,
//        .s = mv->s2
//    };
//    void *v = g_hash_table_lookup(tabu->banned, &key);
//    if (!v)
//        return true;
//    tabu_list_entry_value *value = (tabu_list_entry_value *) v;
////    debug("Found (%d %d %d %d), was banned at time %d (now is time %d -> allowed = %s)",
////          key.c, key.r, key.d, key.s, value->time, time,
////          BOOL_TO_STR(value->time + tabu->tenure > time));
//
//    return value->time + tabu->tenure < time;
//}
//
//void tabu_list_insert(tabu_list *tabu, neighbourhood_swap_move *mv, int time) {
////    debug("Banning (%d %d %d %d) at time %d", mv->c1, mv->r1, mv->d1, mv->s1, time);
//    tabu_list_entry_key *key = mallocx(1, sizeof(tabu_list_entry_key));
//    key->c = mv->c1;
//    key->r = mv->r1;
//    key->d = mv->d1;
//    key->s = mv->s1;
//    key->tabu = tabu;
//
//    tabu_list_entry_value *value;
//
//    void *v = g_hash_table_lookup(tabu->banned, key);
//    if (v) {
//        value = (tabu_list_entry_value *) v;
//    }
//    else {
//        value = mallocx(1, sizeof(tabu_list_entry_value));
//        g_hash_table_insert(tabu->banned, key, value);
//    }
//
//    value->time = time;
//}

//bool tabu_list_is_allowed(tabu_list *tabu, neighbourhood_swap_move *mv, int time) {
//    for (int i = 0; i < tabu->len; i++) {
//        const tabu_list_entry *entry = &tabu->banned[i];
//        if (mv->c1 == entry->c && mv->r2 == entry->r &&
//            mv->d2 == entry->d && mv->s2 == entry->s)
//            return true;
//    }
//    return false;
//}

//void tabu_list_dump(tabu_list *tabu) {
//    for (int i = 0; i < tabu->len; i++) {
//        const tabu_list_entry *entry = &tabu->banned[i];
//        debug("Tabu[%d] (c=%d, r=%d, d=%d, s=%d)%s", i,
//              entry->c, entry->r, entry->d, entry->s, (tabu->cursor + tabu->len - 1) % tabu->len == i ? "   // last" : "");
//    }
//}

//bool tabu_list_insert(tabu_list *tabu, neighbourhood_swap_move *mv, bool assert_valid) {
//    tabu_list_dump(tabu);
//    if (assert_valid)
//        assert(!tabu_list_contains(tabu, mv));
//    tabu->banned[tabu->cursor].c = mv->c1;
//    tabu->banned[tabu->cursor].r = mv->r1;
//    tabu->banned[tabu->cursor].d = mv->d1;
//    tabu->banned[tabu->cursor].s = mv->s1;
//    tabu->cursor = (tabu->cursor + 1) % tabu->len;
//}


typedef struct hill_climbing_solver_state {
    solution *current_solution;
    solution *best_solution;

    int current_cost;
    int best_cost;
    int cycle;

    tabu_list tabu;
} hill_climbing_solver_state;

typedef bool (*neighbourhood_swap_acceptor)(
        neighbourhood_swap_result /* swap result */,
        hill_climbing_solver_state *state,
        void * /* arg */);

typedef bool (*neighbourhood_swap_acceptor2)(
        neighbourhood_swap_result /* swap result 1*/,
        neighbourhood_swap_result /* swap result 2*/,
        hill_climbing_solver_state *state,
        void * /* arg */);


typedef bool (*hill_climber_step)(
        hill_climbing_solver_state *,
        neighbourhood_swap_acceptor,
        void *);

static bool eventually_update_best_solution(hill_climbing_solver_state *state) {
    if (state->current_cost < state->best_cost) {
        state->best_cost = state->current_cost;
        solution_copy(state->best_solution, state->current_solution);
        verbose("[%d] Best solution found = %d", state->cycle, state->best_cost);
        if (state->best_cost < 0)
            eprint("WTF");
        return true;
    }
    return false;
}

static bool hill_climbing_local_search_depth_2_phase(
        hill_climbing_solver_state *state,
        neighbourhood_swap_acceptor accept1,
        neighbourhood_swap_acceptor2 accept2,
        void *acceptor_arg) {

    bool idling;

//    do {
//        idling = true;

    neighbourhood_swap_iter swap_iter1;
    neighbourhood_swap_iter_init(&swap_iter1, state->current_solution);
    neighbourhood_swap_move swap_mv1, swap_mv2;
    neighbourhood_swap_result swap_res1, swap_res2;

    while (neighbourhood_swap_iter_next(&swap_iter1, &swap_mv1)) {
        neighbourhood_swap(state->current_solution, &swap_mv1,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PERFORM_NEVER,
                           &swap_res1);

        if (swap_res1.feasible && accept1(swap_res1, state, acceptor_arg)) {
            neighbourhood_swap(state->current_solution, &swap_mv1,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);
            state->current_cost += swap_res1.delta_cost;
            eventually_update_best_solution(state);

            // --- swap 2 ---
            neighbourhood_swap_iter swap_iter2;
            neighbourhood_swap_iter_init(&swap_iter2, state->current_solution);

            while (neighbourhood_swap_iter_next(&swap_iter2, &swap_mv2)) {
                debug2("hill_climbing_local_search_depth_2_phase %d.%d", swap_iter1.i, swap_iter2.i);

                neighbourhood_swap(state->current_solution, &swap_mv2,
                                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                                   NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                                   NEIGHBOURHOOD_PREDICT_NEVER,
                                   NEIGHBOURHOOD_PERFORM_NEVER,
                                   &swap_res2);
                if (swap_res2.feasible && accept2(swap_res1, swap_res2, state, acceptor_arg)) {
                    debug("Performing a double move of cost %d", swap_res1.delta_cost + swap_res2.delta_cost);
                    neighbourhood_swap(state->current_solution, &swap_mv2,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PERFORM_ALWAYS,
                                       &swap_res2);
                    state->current_cost += swap_res1.delta_cost + swap_res2.delta_cost;
                    eventually_update_best_solution(state);
                    idling = false;
                }
            }

            neighbourhood_swap_iter_destroy(&swap_iter2);

            // --------------
            if (!idling)
                break;

            neighbourhood_swap_back(state->current_solution, &swap_mv1, &swap_res1, false);
        }
    }
    neighbourhood_swap_iter_destroy(&swap_iter1);
//    } while (!idling);
}


//
//
//typedef struct tabu_list_swap_move_stat_key {
////    const tabu_list *tabu;
////    neighbourhood_swap_move move;
//    int c;
//    int r;
//    int d;
//    int s;
//} tabu_list_swap_move_stat_key;
//
//typedef struct tabu_list_swap_move_stat_value {
//    int time;
//
//    // useless
//#if WEIGHTED_MEAN
//    double weighted_delta_mean;
//    int count;
//    int delta_sum;
//#else
//    int count;
//    int delta_sum;
//#endif
//} tabu_list_swap_move_stat_value;
//

static void apply_solution_fingerprint_diff(solution_fingerprint_t *f,
                                     solution_fingerprint_t diff_plus,
                                     solution_fingerprint_t diff_minus) {
    f->sum += diff_plus.sum;
    f->sum -= diff_minus.sum;
    f->xor ^= diff_plus.xor;
    f->xor ^= diff_minus.xor;
}

static void hill_climbing_tabu_search(hill_climbing_solver_state *state, int max_idle) {
    int target_cost = state->current_cost;

    int iter = 0;
    int idle = 0;

    solution_fingerprint_t current_fingerprint = solution_fingerprint(state->current_solution);

    do {
        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, state->current_solution);

        neighbourhood_swap_move swap_mv, best_swap_mv;
        neighbourhood_swap_result swap_result, best_swap_result;
        solution_fingerprint_t best_swap_fingerprint;
        best_swap_result.delta_cost = INT_MAX;

        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            solution_fingerprint_t f = current_fingerprint;
            neighbourhood_swap(state->current_solution, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);
            apply_solution_fingerprint_diff(&f, swap_result.fingerprint_plus, swap_result.fingerprint_minus);

            if (swap_result.feasible && !solution_fingerprint_equal(f, current_fingerprint) &&
                    swap_result.delta_cost < best_swap_result.delta_cost &&
                    (state->current_cost + swap_result.delta_cost < state->best_cost ||
                    tabu_list_is_allowed(&state->tabu, &swap_mv, iter))) {
                best_swap_result = swap_result;
                best_swap_mv = swap_mv;
                best_swap_fingerprint = f;
            }
        }

        neighbourhood_swap_iter_destroy(&swap_iter);

//        debug("TS move (c=%d, r=%d, d=%d, s=%d  <->  c=%d, r=%d, d=%d, s=%d) of cost %d",
//              best_swap_mv.c1, best_swap_mv.r1, best_swap_mv.d1, best_swap_mv.s1,
//              best_swap_mv.c2, best_swap_mv.r2, best_swap_mv.d2, best_swap_mv.s2,
//              best_swap_result.delta_cost);

        if (best_swap_result.delta_cost != INT_MAX) {
            neighbourhood_swap(state->current_solution, &best_swap_mv,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               NULL);

            state->current_cost += best_swap_result.delta_cost;
            current_fingerprint = best_swap_fingerprint;
            bool improved = eventually_update_best_solution(state);
            tabu_list_insert(&state->tabu, &best_swap_mv, &best_swap_result, iter, !improved);
        }

        iter++;

        if (state->current_cost < target_cost) {
            target_cost = state->current_cost;
            idle = 0;
        }
        else {
            idle++;
        }
//        if (iter % 50 == 0)
//            tabu_list_dump(&state->tabu);
        if (idle > 0 && idle % (max_idle / 10) == 0)
            debug("[Iter %d | Idle %d/%d] TS current = %d // best is %d", iter, idle, max_idle, state->current_cost, state->best_cost);
    } while (idle < max_idle);
}

static bool hill_climbing_local_search_phase(
        hill_climbing_solver_state *state,
        neighbourhood_swap_acceptor accept,
        void *acceptor_arg) {
    bool idling;
    do {
        idling = true;

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

            if (swap_result.feasible && accept(swap_result, state, acceptor_arg)) {
                neighbourhood_swap(state->current_solution, &swap_mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   NULL);
                state->current_cost += swap_result.delta_cost;
                eventually_update_best_solution(state);
                idling = false;
                break;
            }
        }

        neighbourhood_swap_iter_destroy(&swap_iter);
    } while (!idling);
}


static bool hill_climbing_neighbourhood_stabilize_room_phase(hill_climbing_solver_state *state) {
    bool idling;
    do {
        idling = true;

        neighbourhood_stabilize_room_iter stabilize_room_iter;
        neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, state->current_solution);

        neighbourhood_stabilize_room_move stabilize_room_mv;
        neighbourhood_stabilize_room_result stabilize_room_result;

        while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter, &stabilize_room_mv)) {
            neighbourhood_stabilize_room(state->current_solution, &stabilize_room_mv,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &stabilize_room_result);

            if (stabilize_room_result.delta_cost < 0) {
                neighbourhood_stabilize_room(state->current_solution, &stabilize_room_mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   NULL);
                state->current_cost += stabilize_room_result.delta_cost;
                eventually_update_best_solution(state);
                idling = false;
                break;
            }
        }

        neighbourhood_stabilize_room_iter_destroy(&stabilize_room_iter);
    } while (!idling);
}



bool hill_climbing_try_random_step(hill_climbing_solver_state *state,
                                   neighbourhood_swap_acceptor accept,
                                   void *acceptor_arg) {
    CRDSQT(state->current_solution->model);

    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result;

    neighbourhood_swap_generate_random_move(state->current_solution, &swap_mv);
    neighbourhood_swap(state->current_solution, &swap_mv,
                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                       NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PERFORM_NEVER,
                       &swap_result);
    if (!swap_result.feasible || !accept(swap_result, state, acceptor_arg))
        return false; // doing nothing

    neighbourhood_swap(state->current_solution, &swap_mv,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);

    state->current_cost += swap_result.delta_cost;
    eventually_update_best_solution(state);

    return true;
}

#define SA_ACCEPTANCE(delta, temperature) pow(M_E, - (double) (delta) / (temperature))

static bool sa_acceptor(
        neighbourhood_swap_result res,
        hill_climbing_solver_state *state,
        void *arg) {
    if (state->current_cost + res.delta_cost < state->best_cost)
        return true; // always accept new optimal

    double temperature = *((double *) arg);
    double p = SA_ACCEPTANCE(res.delta_cost, temperature);
//    debug("SA acceptance probability = %g (T=%g, delta = %d)", p, temperature, res.delta_cost);
    double r = rand_uniform(0, 1);
    return r < p;
}

static void hill_climbing_simulated_annealing_phase(hill_climbing_solver_state *state, double T) {
    const double CR = 0.96;
    const double MIN_T = 0.08;
    const int TEMPERATURE_LENGTH = 25000;

    int idle = 0;
    int target_cost = state->current_cost;
    int iter = 0;
    while (T > MIN_T) {
//        debug("[SA iter %d | Temperature %g | p(+1)=%g%%, p(+10)=%g%% p(+100)=%g%%] "
//              "Current=%d, Best=%d", iter, T,
//              SA_ACCEPTANCE(1, T) * 100, SA_ACCEPTANCE(10, T) * 100, SA_ACCEPTANCE(100, T) * 100,
//              state->current_cost, state->best_cost);
        for (int it = 0; it < TEMPERATURE_LENGTH; it++) {
            hill_climbing_try_random_step(state, sa_acceptor, &T);
            if (state->current_cost < target_cost) {
                target_cost = state->current_cost;
                idle = 0;
            } else {
                idle++;
            }
        }
        iter++;
        T *= CR;
    }
}
//
//static void hill_climbing_simulated_annealing(hill_climbing_solver_state *state, int reheat_phases) {
//    const double REHEAT_COEFF = 1.2;
//    const double INITIAL_T = 2;
//
//    for (int phase = 0; phase < reheat_phases; phase++) {
//        double T = INITIAL_T * pow(REHEAT_COEFF, phase);
//        debug("[SA phase %d | Temperature %g | p(+1)=%g%%, p(+10)=%g%% p(+100)=%g%%] "
//              "Current=%d, Best=%d", phase, T,
//              SA_ACCEPTANCE(1, T) * 100, SA_ACCEPTANCE(10, T) * 100, SA_ACCEPTANCE(100, T) * 100,
//              state->current_cost, state->best_cost);
//        hill_climbing_simulated_annealing_phase(state, T);
//    }
//}

static void hill_climbing_simulated_annealing(hill_climbing_solver_state *state, double T) {
//    const double REHEAT_COEFF = 1.2;
//    const double INITIAL_T = 2;
//
//    for (int phase = 0; phase < reheat_phases; phase++) {
//        double T = INITIAL_T * pow(REHEAT_COEFF, phase);
    debug("[SA phase | Temperature %g | p(+1)=%g%%, p(+10)=%g%% p(+100)=%g%%] "
          "Current=%d, Best=%d", T,
          SA_ACCEPTANCE(1, T) * 100, SA_ACCEPTANCE(10, T) * 100, SA_ACCEPTANCE(100, T) * 100,
          state->current_cost, state->best_cost);
    hill_climbing_simulated_annealing_phase(state, T);
//    }
}

void hill_climbing_random_phase(hill_climbing_solver_state *state,
                                hill_climber_step step,
                                int max_performed_step, int max_step_idle,
                                neighbourhood_swap_acceptor accept,
                                void * acceptor_arg) {
    int performed = 0;
    int idle = 0;
    while (idle <= max_step_idle && performed <= max_performed_step) {
        int prev_cost = state->current_cost;
        performed += step(state, accept, acceptor_arg);
        idle = state->current_cost < prev_cost ? 0 : idle + 1;
        if (idle > 0 && idle % (max_step_idle / 10) == 0)
            debug("[Idle %d/%d] HC current = %d // best is %d", idle, max_step_idle, state->current_cost, state->best_cost);
    }
}

static bool neighbourhood_swap_accept_less_cost(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return state->current_cost + result.delta_cost < state->best_cost;
}

static bool neighbourhood_swap_accept_always(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return true;
}

static bool neighbourhood_swap_accept_less_or_equal_0(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept2_less_or_equal_cost(neighbourhood_swap_result result1, neighbourhood_swap_result result2,
                                                 hill_climbing_solver_state *state, void *arg) {
    return state->current_cost + result1.delta_cost + result2.delta_cost < state->best_cost;
}

static bool neighbourhood_swap_accept_cost_tolerance(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    double tolerance = *((double *) arg);
    return state->current_cost + result.delta_cost <= state->best_cost * tolerance;
}

static bool neighbourhood_swap_accept_less_or_equal_cost(neighbourhood_swap_result result,
                                                         hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rc_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_capacity <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_cc_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_curriculum_compactness <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_mwd_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_min_working_days <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rs_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_stability <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_cost_greedy(neighbourhood_swap_result result,
                                                         hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rc_cost_greedy(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_capacity <= 0 && result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_cc_cost_greedy(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_curriculum_compactness <= 0 && result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_mwd_cost_greedy(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_min_working_days <= 0 && result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rs_cost_greedy(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_stability <= 0 && result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rc_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_room_capacity <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_cc_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_curriculum_compactness <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_min_working_days <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_rs_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_room_stability <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static neighbourhood_swap_acceptor random_acceptor_greedy() {
    static neighbourhood_swap_acceptor acceptors[] = {
//        neighbourhood_swap_accept_less_or_equal_cost,
//        neighbourhood_swap_accept_less_or_equal_rc_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_rs_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_rc_cost_greedy,
        neighbourhood_swap_accept_less_or_equal_rs_cost_greedy,
        neighbourhood_swap_accept_less_or_equal_cc_cost_greedy,
        neighbourhood_swap_accept_less_or_equal_mwd_cost_greedy,
    };

    return acceptors[rand_int_range(0, LENGTH(acceptors))];
}

static neighbourhood_swap_acceptor random_acceptor_bounded() {
    static neighbourhood_swap_acceptor acceptors[] = {
//        neighbourhood_swap_accept_less_or_equal_cost,
        neighbourhood_swap_accept_less_or_equal_rc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_rs_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
//        neighbourhood_swap_accept_less_or_equal_rc_cost_greedy,
//        neighbourhood_swap_accept_less_or_equal_rs_cost_greedy,
//        neighbourhood_swap_accept_less_or_equal_cc_cost_greedy,
//        neighbourhood_swap_accept_less_or_equal_mwd_cost_greedy,
    };

    return acceptors[rand_int_range(0, LENGTH(acceptors))];
}

bool hill_climbing_solver_solve(hill_climbing_solver *solver,
                               hill_climbing_solver_config *config,
                               solution *s) {
    hill_climbing_solver_reinit(solver);

    long starting_time = ms();
    long time_limit = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int idle_limit = config->idle_limit;

    if (config->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(s, config->starting_solution);
    }
    else {
        debug("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

        feasible_solution_finder_find(&finder, &finder_config, s);
    }

    solution best_solution;
    solution_init(&best_solution, s->model);
    solution_copy(&best_solution, s);

    int initial_cost = solution_cost(s);

    hill_climbing_solver_state state = {
        .current_solution = s,
        .current_cost = initial_cost,
        .best_solution = &best_solution,
        .best_cost = initial_cost,
    };

    state.cycle = 0;

    int non_increasing_iters = 0;
    int non_best_iters = 0;
    int last_restart_iter = INT_MAX;
    double reheat_coeff = 1.2;

    while (true) {
        if (time_limit > 0 && !(ms() < time_limit)) {
            verbose("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }
//        assert(state.current_cost == solution_cost(state.current_solution));

        verbose("[%d] -------- MAJOR CYCLE BEGIN --------", state.cycle);
        verbose("[%d] Cost = %d                   // best = %d",
                state.cycle, state.current_cost, state.best_cost);
        verbose("[%d] Non increasing iters = %d",
                state.cycle, non_increasing_iters);
        verbose("[%d] Non best iters = %d",
                state.cycle, non_best_iters);
        verbose("[%d] Distance from best = %g%%",
                state.cycle, (double) 100 * state.current_cost / state.best_cost);
        verbose("[%d] SA Reheat = %g",
                state.cycle, reheat_coeff);

        int prev_best_cost = state.best_cost;
        int prev_current_cost = state.current_cost;
// Phase PHASE_LS
#define PHASE_LS 0
#if PHASE_LS
            debug("[%d] Phase PHASE_LS", state.cycle);
            hill_climbing_local_search_phase(&state,
                                             neighbourhood_swap_accept_less_cost,
                                             NULL);
            debug("[%d] Cost PHASE_LS END = %d      // best = %d",
                  state.cycle, state.current_cost, state.best_cost);
#endif

#define PHASE_HC_SWAP_MOVE 0
#if PHASE_HC_SWAP_MOVE
        verbose("[%d] HILL CLIMBING:            cost = %d // best = %d",
                state.cycle, state.current_cost, state.best_cost);
        hill_climbing_random_phase(&state,
                            hill_climbing_try_random_step, INT_MAX, 200000,
                            neighbourhood_swap_accept_less_or_equal_cost, NULL);
        verbose("[%d] HILL CLIMBING END:        cost = %d // best = %d",
                state.cycle, state.current_cost, state.best_cost);
#endif

// Phase PHASE_STAB_ROOM
#define PHASE_STAB_ROOM 0
#if PHASE_STAB_ROOM
            debug("[%d] Phase PHASE_STAB_ROOM: hill_climbing_neighbourhood_stabilize_room_phase", state.cycle);
            hill_climbing_neighbourhood_stabilize_room_phase(&state);
            debug("[%d] Cost after phase [hill_climbing_neighbourhood_stabilize_room_phase] = %d      // best = %d",
                  state.cycle, state.current_cost, state.best_cost);
#endif

// Phase PHASE_SA
#define PHASE_SA 1
#if PHASE_SA
        verbose("[%d] SIMULATED ANNEALING:      cost = %d // best = %d",
                state.cycle, state.current_cost, state.best_cost);
        hill_climbing_simulated_annealing(&state, 1.8 * pow(reheat_coeff, non_increasing_iters));
        verbose("[%d] SIMULATED ANNEALING END:  cost = %d // best = %d",
                state.cycle, state.current_cost, state.best_cost);
#endif
// Phase PHASE_TS
#define PHASE_TS 0
#if PHASE_TS
        const double TT_MAX = 14;
        const double TT_MIN = 5;
        int tenure = (int) map(state.best_cost, initial_cost, TT_MIN, TT_MAX, state.current_cost);
        verbose("[%d] PHASE_TS [tenure=%d]", state.cycle, tenure);
        tabu_list_init(&state.tabu, state.current_solution->model, tenure);
        hill_climbing_tabu_search(&state, 300);
        tabu_list_destroy(&state.tabu);
        tabu_list_dump(&state.tabu);
        verbose("[%d] PHASE_TS END, cost = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif


        if (state.current_cost < prev_current_cost) {
            non_increasing_iters = 0;
        }
        else {
            non_increasing_iters++;
        }

        if (state.best_cost < prev_best_cost) {
            non_best_iters = 0;
            reheat_coeff = 1.5;
        } else {
            const int RESTART_AFTER = 15;
            if (non_best_iters < RESTART_AFTER) {
                non_best_iters++;
            } else {
                verbose("[%d] !!! RESTART FROM BEST (%d)",
                        state.cycle, state.best_cost);
                non_best_iters = 0;
                non_increasing_iters = 0;
                solution_copy(state.current_solution, state.best_solution);
                state.current_cost = state.best_cost;
                reheat_coeff *= 1.5;
            }
        }

        // Phase 2
#define PHASE_HC_RND_MOVE 1
#if PHASE_HC_RND_MOVE
        neighbourhood_swap_acceptor acceptor = random_acceptor_bounded();

        static const int UNCHECKED_MOVES_UB = 5;
        static const int UNCHECKED_MOVES_LB = 5;

        double max_allowed_cost_factor =
                map((double) starting_time, (double) time_limit, 1.5, 1.1, (double) ms());
        int max_allowed_cost = (int) (state.current_cost * max_allowed_cost_factor);

        int n_moves = 10;
        verbose("[%d] PHASE_HC_RND_MOVE - %s move (n_moves=%d, max_allowed_cost=%d)", state.cycle,
              (acceptor == neighbourhood_swap_accept_less_or_equal_rc_cost_bounded ? "RoomCapacity" :
               (acceptor == neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded ? "MinWorkingDays" :
                (acceptor == neighbourhood_swap_accept_less_or_equal_cc_cost_bounded ? "CurriculumCompactness" :
                 (acceptor == neighbourhood_swap_accept_less_or_equal_rs_cost_bounded ? "RoomStability" :
                  (acceptor == neighbourhood_swap_accept_less_or_equal_cost ? "Swap" : "Unknown"))))),
              n_moves, max_allowed_cost);
        hill_climbing_random_phase(&state,
                                   hill_climbing_try_random_step, n_moves, n_moves * 10,
                                   acceptor, &max_allowed_cost);
        verbose("[%d] PHASE_HC_RND_MOVE END, cost = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif
        state.cycle++;
    }

    solution_copy(s, &best_solution);

    return strempty(solver->error);
}
