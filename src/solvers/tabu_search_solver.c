#include "tabu_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <utils/mem_utils.h>
#include <utils/random_utils.h>
#include <math.h>
#include <utils/math_utils.h>
#include <neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/array_utils.h>
#include "neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"
#include "utils/assert_utils.h"

//typedef struct solution_tabu_fingerprint {
//    int room_capacity_cost;
//    int min_working_days_cost;
//    int curriculum_compactness_cost;
//    int room_stability_cost;
//} solution_tabu_fingerprint;
//typedef unsigned long long solution_tabu_fingerprint;

typedef struct tabu_list {
    const model *model;

    solution_fingerprint_t *solutions;
    size_t solutions_len;
    size_t solution_index;

    neighbourhood_swap_move *swaps;
    size_t swaps_len;
    size_t swap_index;

    GHashTable *swap_stats;
    GHashTable *stabilize_room_stats;
} tabu_list;


static const double ONE_THIRD = ((double) 1 / 2);
static const double TWO_THIRD = ((double) 1 / 2);

#define WEIGHTED_MEAN 1
#define N2 0

typedef struct tabu_list_swap_move_stat_key {
    const tabu_list *tabu;
//    neighbourhood_swap_move move;
    int c;
    int r;
    int d;
    int s;
} tabu_list_swap_move_stat_key;

typedef struct tabu_list_swap_move_stat_value {
    int time;

    // useless
#if WEIGHTED_MEAN
    double weighted_delta_mean;
    int count;
    int delta_sum;
#else
    int count;
    int delta_sum;
#endif
} tabu_list_swap_move_stat_value;

static guint tabu_list_swap_move_stat_hash(gconstpointer  key) {
    const tabu_list_swap_move_stat_key *stat_key = (tabu_list_swap_move_stat_key *) key;
    CRDSQT(stat_key->tabu->model);
    return INDEX4(stat_key->c, C, stat_key->r, R, stat_key->d, D, stat_key->s, S);
//    uint i1 = INDEX4(stat_key->move.c1, move_stat->tabu->model->n_courses,
//                  stat_key->move.r1, stat_key->tabu->model->n_rooms,
//                  stat_key->move.d1, stat_key->tabu->model->n_days,
//                  stat_key->move.s1, stat_key->tabu->model->n_slots);
//    uint i2 = INDEX4(stat_key->move._c2, move_stat->tabu->model->n_courses,
//                  stat_key->move.r2, stat_key->tabu->model->n_rooms,
//                  stat_key->move.d2, stat_key->tabu->model->n_days,
//                  stat_key->move.s2, stat_key->tabu->model->n_slots);
//    return i1 + i2;
}

static gboolean tabu_list_swap_move_stat_equal(gconstpointer  a, gconstpointer  b) {
    const tabu_list_swap_move_stat_key *move_stat_a = (tabu_list_swap_move_stat_key *) a;
    const tabu_list_swap_move_stat_key *move_stat_b = (tabu_list_swap_move_stat_key *) b;
    return
        move_stat_a->c == move_stat_b->c &&
        move_stat_a->r == move_stat_b->r &&
        move_stat_a->d == move_stat_b->d &&
        move_stat_a->s == move_stat_b->s;
//
//    return
//        (move_stat_a->move.c1 == move_stat_b->move.c1 &&
//        move_stat_a->move.r1 == move_stat_b->move.r1 &&
//        move_stat_a->move.d1 == move_stat_b->move.d1 &&
//        move_stat_a->move.s1 == move_stat_b->move.s1 &&
//        move_stat_a->move._c2 == move_stat_b->move._c2 &&
//        move_stat_a->move.r2 == move_stat_b->move.r2 &&
//        move_stat_a->move.d2 == move_stat_b->move.d2 &&
//        move_stat_a->move.s2 == move_stat_b->move.s2) ||
//        (move_stat_a->move.c1 == move_stat_b->move._c2 &&
//        move_stat_a->move.r1 == move_stat_b->move.r2 &&
//        move_stat_a->move.d1 == move_stat_b->move.d2 &&
//        move_stat_a->move.s1 == move_stat_b->move.s2 &&
//        move_stat_a->move._c2 == move_stat_b->move.c1 &&
//        move_stat_a->move.r2 == move_stat_b->move.r1 &&
//        move_stat_a->move.d2 == move_stat_b->move.d1 &&
//        move_stat_a->move.s2 == move_stat_b->move.s1);
}


typedef struct tabu_list_stabilize_room_move_stat_key {
    const tabu_list *tabu;
    neighbourhood_stabilize_room_move move;
} tabu_list_stabilize_room_move_stat_key;

typedef struct tabu_list_stabilize_room_move_stat_value {
#if WEIGHTED_MEAN
    double weighted_delta_mean;
#else
    int count;
    int delta_sum;
#endif
} tabu_list_stabilize_room_move_stat_value;

static guint tabu_list_stabilize_room_move_stat_hash(gconstpointer  key) {
    const tabu_list_stabilize_room_move_stat_key *stat_key = (tabu_list_stabilize_room_move_stat_key *) key;

    return INDEX2(stat_key->move.c1, move_stat->tabu->model->n_courses,
                  stat_key->move.r2, stat_key->tabu->model->n_rooms);
}

static gboolean tabu_list_stabilize_room_move_stat_equal(gconstpointer  a, gconstpointer  b) {
    const tabu_list_stabilize_room_move_stat_key *move_stat_a = (tabu_list_stabilize_room_move_stat_key *) a;
    const tabu_list_stabilize_room_move_stat_key *move_stat_b = (tabu_list_stabilize_room_move_stat_key *) b;

    return
        (move_stat_a->move.c1 == move_stat_b->move.c1 &&
        move_stat_a->move.r2 == move_stat_b->move.r2);
}

static void tabu_list_init(tabu_list *tabu, size_t tabu_size, const model *model) {
    debug("Initializing tabu list of size %lu", tabu_size);
    tabu->solutions = callocx(tabu_size, sizeof(solution_fingerprint_t));
    tabu->solutions_len = tabu_size;
    tabu->solution_index = 0;
    tabu->swaps = callocx(tabu_size, sizeof(neighbourhood_swap_move));
    tabu->swaps_len = tabu_size;
    tabu->swap_index = 0;

    tabu->swap_stats = g_hash_table_new_full(
            tabu_list_swap_move_stat_hash, tabu_list_swap_move_stat_equal, free, free);
    tabu->stabilize_room_stats = g_hash_table_new_full(
            tabu_list_stabilize_room_move_stat_hash, tabu_list_stabilize_room_move_stat_equal, free, free);
    tabu->model = model;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->solutions);
    g_hash_table_destroy(tabu->swap_stats);
}

static bool tabu_list_solution_is_allowed(const tabu_list *tabu, solution_fingerprint_t f) {
    for (int i = 0; i < tabu->solutions_len; i++) {
        if (solution_fingerprint_equal(f, tabu->solutions[i])) {
            debug2("TABU forbids {s=%llu, p=%llu}", f.sum, f.xor);
            return false;
        }
    }
    return true;
}
#if 0
static bool tabu_list_swap_move_is_allowed(const tabu_list *tabu, neighbourhood_swap_move move) {
    for (int i = 0; i < tabu->swaps_len; i++) {
        const neighbourhood_swap_move *swap =  &tabu->swaps[i];
        if (
//                neighbourhood_swap_move_equal(&move, swap) ||
            neighbourhood_swap_move_same_course(&move, swap) &&
//            neighbourhood_swap_move_same_room(&move, swap) ||
            neighbourhood_swap_move_same_period(&move, swap))
            return false;
    }
    return true;
}


static void tabu_list_insert_swap_move(tabu_list *tabu, neighbourhood_swap_move swap_move, int t) {
    tabu->swaps[tabu->swap_index] = swap_move;
    tabu->swap_index = (tabu->swap_index + 1) % tabu->swaps_len;
}

#endif

static void tabu_list_insert_solution(tabu_list *tabu, solution_fingerprint_t f) {
    debug("tabu_list_insert_solution [%lu]=(s=%llu, p=%llu)", tabu->solution_index, f.sum, f.xor);

#ifdef ASSERT
    for (int i = 0; i < tabu->solutions_len; i++) {
        assert(!solution_fingerprint_equal(tabu->solutions[i], f));
    }
#endif

    tabu->solutions[tabu->solution_index] = f;
    tabu->solution_index = (tabu->solution_index + 1) % tabu->solutions_len;
}

static int tabu_list_move_expiration(int t, int count) {
    if (count == 0)
        return 0;
    return (int) (t + pow(2, count));
}

static void tabu_list_insert_swap_move_part(tabu_list *tabu, int c, int r, int d, int s, int t) {
    tabu_list_swap_move_stat_key *stat_key = mallocx(1, sizeof(tabu_list_swap_move_stat_key));
//    neighbourhood_swap_move_copy(&stat_key->move, &swap_move);
    stat_key->c = c;
    stat_key->r = r;
    stat_key->d = d;
    stat_key->s = s;
    stat_key->tabu = tabu;

    tabu_list_swap_move_stat_value * stat_val;
    int prev_t = 0;
    int prev_count = 0;
    void *v = g_hash_table_lookup(tabu->swap_stats, stat_key);
    if (v) {
        stat_val = (tabu_list_swap_move_stat_value *) v;
        prev_t = stat_val->time;
        prev_count = stat_val->count;
        free(stat_key);
    }
    else {
        stat_val = mallocx(1, sizeof(tabu_list_swap_move_stat_value));
        stat_val->count = 0;
        g_hash_table_insert(tabu->swap_stats, stat_key, stat_val);
    }

    stat_val->count++;
    stat_val->time = t;

    if (t < tabu_list_move_expiration(prev_t, prev_count)) {
        debug("Using tabu move! t is %d, expiration is =%d", t, tabu_list_move_expiration(prev_t, prev_count));
        exit(1);
    }
    debug("tabu_list_insert_swap_move (c=%d r=%d d=%d s=%d) -> [count=%d, t=%d, sum=%d] "
          "(t was %d, tabu expired on t=%d)",
          c, r, d, s,
          stat_val->count, stat_val->time, stat_val->delta_sum, prev_t,
          tabu_list_move_expiration(prev_t, prev_count));
}

static void tabu_list_insert_swap_move(tabu_list *tabu, neighbourhood_swap_move mv, int t) {
    tabu_list_insert_swap_move_part(tabu, mv.c1, mv.r1, mv.d1, mv.s1, t);
    tabu_list_insert_swap_move_part(tabu, mv._c2, mv.r2, mv.d2, mv.s2, t);
}

static bool tabu_list_swap_move_is_allowed_part(const tabu_list *tabu, int c, int r, int d, int s, int t) {
    tabu_list_swap_move_stat_key k;
    k.tabu = tabu;
    k.c = c;
    k.r = r;
    k.d = d;
    k.s = s;

    void * v = g_hash_table_lookup(tabu->swap_stats, &k);
    if (!v)
        return true;

    tabu_list_swap_move_stat_value *vv = (tabu_list_swap_move_stat_value *) v;

    return t > tabu_list_move_expiration(vv->time, vv->count);
}

static bool tabu_list_swap_move_is_allowed(const tabu_list *tabu, neighbourhood_swap_move mv, int t) {
    return
        tabu_list_swap_move_is_allowed_part(tabu, mv.c1, mv.r1, mv.d1, mv.s1, t) &&
        tabu_list_swap_move_is_allowed_part(tabu, mv._c2, mv.r2, mv.d2, mv.s2, t);
}



static double tabu_list_swap_cost(const tabu_list *tabu, neighbourhood_swap_move move,
                                  neighbourhood_swap_result result, solution_fingerprint_t fingerprint) {
    if (!tabu_list_solution_is_allowed(tabu, fingerprint))
        return DBL_MAX;

    tabu_list_swap_move_stat_key k;
    k.tabu = tabu;
//    k.move = move;

    void * v = g_hash_table_lookup(tabu->swap_stats, &k);
    if (v)
        return DBL_MAX;
    return result.delta_cost;

    static double D = 5;
    double penalty = 0;
    if (v) {
        tabu_list_swap_move_stat_value *vv = (tabu_list_swap_move_stat_value *) v;
#if WEIGHTED_MEAN
        double weighted_mean = vv->weighted_delta_mean;
        penalty = weighted_mean;

        debug2("tabu_list_swap_cost (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> [w_mean=%g] -> %g",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          vv->weighted_delta_mean, result.delta_cost + D * penalty);
#else
        double delta_mean = (double) (vv->delta_sum + vv->count) / (vv->count);
        penalty = delta_mean;
        debug("tabu_list_swap_cost (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> [count=%d, delta_mean=%g] -> %g",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          vv->count, delta_mean, result.delta_cost + D * penalty);
#endif

    }

    debug2("tabu_list_swap_cost (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> %g",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,  result.delta_cost + D * penalty);

    return result.delta_cost + D * penalty;
}



static double tabu_list_stabilize_room_cost(
        const tabu_list *tabu, neighbourhood_stabilize_room_move move,
        neighbourhood_stabilize_room_result result, solution_fingerprint_t fingerprint) {
    if (!tabu_list_solution_is_allowed(tabu, fingerprint))
        return DBL_MAX;

    tabu_list_stabilize_room_move_stat_key k;
    k.tabu = tabu;
    k.move = move;

    void * v = g_hash_table_lookup(tabu->stabilize_room_stats, &k);
    if (v)
        return DBL_MAX;
    return result.delta_cost;

    static double D = 5;
    double penalty = 0;
    if (v) {
        tabu_list_stabilize_room_move_stat_value *vv = (tabu_list_stabilize_room_move_stat_value *) v;
#if WEIGHTED_MEAN
        double weighted_mean = vv->weighted_delta_mean;
        penalty = weighted_mean;
        debug("tabu_list_stabilize_room_cost (c=%d r=%d) -> [w_mean=%g] -> %g",
          move.c1, move.r2,
          vv->weighted_delta_mean, result.delta_cost + D * penalty);
#else
        double delta_mean = (double) (vv->delta_sum + vv->count) / (vv->count);
        penalty = delta_mean;

        debug2("tabu_list_stabilize_room_cost (c=%d r=%d) -> [count=%d, delta_mean=%g] -> %g",
          move.c1, move.r2,
          vv->count, delta_mean, result.delta_cost + D * penalty);
#endif
    }

    debug2("tabu_list_stabilize_room_cost (c=%d r=%d) -> %g",
          move.c1, move.r2, result.delta_cost + D * penalty);

    return result.delta_cost + D * penalty;
}


static void tabu_list_update_swap_stats(tabu_list* tabu,
                                        neighbourhood_swap_move move,
                                        neighbourhood_swap_result result) {
    tabu_list_swap_move_stat_key *stat_key = mallocx(1, sizeof(tabu_list_swap_move_stat_key));
//    neighbourhood_swap_move_copy(&stat_key->move, &move);
    stat_key->tabu = tabu;

    tabu_list_swap_move_stat_value * stat_val;
    void *v = g_hash_table_lookup(tabu->swap_stats, stat_key);
    if (v) {
        stat_val = (tabu_list_swap_move_stat_value *) v;
        free(stat_key);
    }
    else {
        stat_val = mallocx(1, sizeof(tabu_list_swap_move_stat_value));
#if WEIGHTED_MEAN
        stat_val->weighted_delta_mean = 0;
        stat_val->count = 0;
        stat_val->delta_sum = 0;
#else
        stat_val->count = 0;
        stat_val->delta_sum = 0;
#endif
        g_hash_table_insert(tabu->swap_stats, stat_key, stat_val);
    }

#if WEIGHTED_MEAN
    double delta_mean = (double) (stat_val->delta_sum + stat_val->count) / (stat_val->count + 1);

    stat_val->weighted_delta_mean = delta_mean + 2 * (double) result.delta_cost / (stat_val->count + 1);

    stat_val->count++;
    stat_val->delta_sum += result.delta_cost;

    debug("tabu_list_update_swap_stats (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) of delta %d -> [count=%d, sum=%d, w_mean=%g]",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          result.delta_cost,
          stat_val->count,
          stat_val->delta_sum,
          stat_val->weighted_delta_mean);
    if (stat_val->count > 2)
        mssleep(0);
#else
    stat_val->count++;
    stat_val->delta_sum += result.delta_cost;

    debug("tabu_list_update_swap_stats (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> [count=%d, sum=%d]",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          stat_val->count, stat_val->delta_sum);
#endif
}

static void tabu_list_update_stabilize_room_stats(tabu_list* tabu,
                                              neighbourhood_stabilize_room_move move,
                                              neighbourhood_stabilize_room_result result) {
    tabu_list_stabilize_room_move_stat_key *stat_key = mallocx(1, sizeof(neighbourhood_stabilize_room_move));
    neighbourhood_stabilize_room_move_copy(&stat_key->move, &move);
    stat_key->tabu = tabu;

    tabu_list_stabilize_room_move_stat_value * stat_val;
    void *v = g_hash_table_lookup(tabu->stabilize_room_stats, stat_key);
    if (v) {
        stat_val = (tabu_list_stabilize_room_move_stat_value *) v;
        free(stat_key);
    }
    else {
        stat_val = mallocx(1, sizeof(tabu_list_stabilize_room_move_stat_value));
#if WEIGHTED_MEAN
        stat_val->weighted_delta_mean = 0;
#else
        stat_val->count = 0;
        stat_val->delta_sum = 0;
#endif
        g_hash_table_insert(tabu->stabilize_room_stats, stat_key, stat_val);
    }

#if WEIGHTED_MEAN
    stat_val->weighted_delta_mean = stat_val->weighted_delta_mean * ONE_THIRD + result.delta_cost * TWO_THIRD;

    debug("tabu_list_update_stabilize_room_stats (c=%d r=%d) -> [w_mean=%g]",
          move.c1, move.r2,
          stat_val->weighted_delta_mean);
#else
    stat_val->count++;
    stat_val->delta_sum += result.delta_cost;

    debug("tabu_list_update_stabilize_room_stats (c=%d r=%d) -> [count=%d, sum=%d]",
          move.c1, move.r2,
          stat_val->count, stat_val->delta_sum);

#endif
}

void tabu_search_solver_config_init(tabu_search_solver_config *config) {
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
    config->tabu_size = 32;
}

void tabu_search_solver_config_destroy(tabu_search_solver_config *config) {

}

void tabu_search_solver_init(tabu_search_solver *solver) {
    solver->error = NULL;
}

void tabu_search_solver_destroy(tabu_search_solver *solver) {
    free(solver->error);
}

const char *tabu_search_solver_get_error(tabu_search_solver *solver) {
    return solver->error;
}

void tabu_search_solver_reinit(tabu_search_solver *solver) {
    tabu_search_solver_destroy(solver);
    tabu_search_solver_init(solver);
}

typedef struct neighbourhood_swap_move_info {
    neighbourhood_swap_move move;
    neighbourhood_swap_result result;
    solution_fingerprint_t fingerprint;
    int cost;
} neighbourhood_swap_move_info;


int neighbourhood_swap_move_result_compare(const void *_1, const void *_2) {
    neighbourhood_swap_move_info *mv_res1 = (neighbourhood_swap_move_info *) _1;
    neighbourhood_swap_move_info *mv_res2 = (neighbourhood_swap_move_info *) _2;

//    return mv_res2->result.delta_cost - mv_res1->result.delta_cost;
    return mv_res2->cost - mv_res1->cost;
}

void apply_solution_fingerprint_diff(solution_fingerprint_t *f,
                                     solution_fingerprint_t diff_plus,
                                     solution_fingerprint_t diff_minus) {
    f->sum += diff_plus.sum;
    f->sum -= diff_minus.sum;
    f->xor ^= diff_plus.xor;
    f->xor ^= diff_minus.xor;
}

#if 1

int tabu_search_solve_phase(int iter, solution *s, solution_fingerprint_t *fingerprint,
                            int *cost, tabu_list *tabu) {
    bool improved;

    do {
        improved = false;

        debug("[%d] TS: phase starting solution {s=%llu, p=%llu) cost =%d",
                   iter, fingerprint->sum, fingerprint->xor, *cost);

        bool n1_improved;
        int i, j;

        i = 0;
        do {
            j = 0;

            n1_improved = false;

            neighbourhood_swap_iter swap_iter;
            neighbourhood_swap_iter_init(&swap_iter, s);

            neighbourhood_swap_result swap_result;
            neighbourhood_swap_move swap_mv;

            neighbourhood_swap_move best_swap_mv;
            neighbourhood_swap_result best_swap_result;
            double best_swap_tabu_cost = DBL_MAX;

            while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
                debug2("[%d.swap.%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                    iter, i, j,
                    swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                    swap_mv.r2, swap_mv.d2, swap_mv.s2);

                neighbourhood_swap(s, &swap_mv,
                    NEIGHBOURHOOD_PREDICT_ALWAYS,
                    NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                    NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                    NEIGHBOURHOOD_PERFORM_NEVER,
                    &swap_result);

                if (swap_result.feasible) {
                    solution_fingerprint_t swap_fingerprint = *fingerprint;
                    apply_solution_fingerprint_diff(&swap_fingerprint,
                             swap_result.fingerprint_plus,
                             swap_result.fingerprint_minus);

                    debug2("[%d.swap.%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) cost = %d -> {s=%llu, p=%llu}",
                        iter, i, j,
                        swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                        swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost,
                        swap_fingerprint.sum, swap_fingerprint.xor);

//                    double tabu_cost = tabu_list_swap_cost(tabu, swap_mv, swap_result, swap_fingerprint);
                    double tabu_cost = swap_result.delta_cost;
//                    double tabu_cost = -neighbourhood_swap_move_cost(&swap_mv, s);


                    if (!solution_fingerprint_equal(*fingerprint, swap_fingerprint) &&
                        tabu_list_swap_move_is_allowed(tabu, swap_mv, iter) &&
                        tabu_list_solution_is_allowed(tabu, swap_fingerprint) &&
                        tabu_cost < best_swap_tabu_cost /* || better than best sol */) {
                        debug2("[%d.swap.%d.%d] TS: new best swap move candidate"
                            "(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) of delta = %d (tabu cost = %g)",
                            iter, i, j,
                            swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                            swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost,
                            tabu_cost);
                        best_swap_tabu_cost = tabu_cost;
                        best_swap_result = swap_result;
                        best_swap_mv = swap_mv;
                    }
                }
                j++;
            }

            if (best_swap_tabu_cost == DBL_MAX) {
                debug("NO MORE MOVES TO DO");
                break;
            }
//            if (best_swap_result.delta_cost < 0) {
//            if (best_swap_result.delta_cost < 0) {
            debug("[%d.%d] TS: using swap move swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) cost = %d",
                iter, i,
                best_swap_mv.c1, best_swap_mv.r1, best_swap_mv.d1, best_swap_mv.s1,
                best_swap_mv.r2, best_swap_mv.d2, best_swap_mv.s2, best_swap_result.delta_cost);
            neighbourhood_swap(s, &best_swap_mv,
                NEIGHBOURHOOD_PREDICT_NEVER,
                NEIGHBOURHOOD_PREDICT_ALWAYS,
                NEIGHBOURHOOD_PREDICT_ALWAYS,
                NEIGHBOURHOOD_PERFORM_ALWAYS,
                &swap_result);
            tabu_list_insert_swap_move(tabu, best_swap_mv, iter);
            apply_solution_fingerprint_diff(fingerprint,
                 swap_result.fingerprint_plus,
                 swap_result.fingerprint_minus);
            *cost += swap_result.delta_cost;
//            }

//            char tmp[64];
//            snprintf(tmp, 64, "[%d] c=%d r=%d d=%d s=%d -> %d\n",
//                     iter,
//                     best_swap_mv.c1, best_swap_mv.r1,
//                     best_swap_mv.d1, best_swap_mv.s1,
//                     best_swap_result.delta_cost);
//            fileappend("/tmp/moves.txt", tmp);
//            snprintf(tmp, 64, "[%d] c=%d r=%d d=%d s=%d -> %d\n",
//                     iter,
//                     best_swap_mv._c2, best_swap_mv.r2,
//                     best_swap_mv.d2, best_swap_mv.s2,
//                     best_swap_result.delta_cost);
//            fileappend("/tmp/moves.txt", tmp);

            if (best_swap_result.delta_cost < 0) {
                n1_improved = true;
            }

            neighbourhood_swap_iter_destroy(&swap_iter);

            i++;
        } while (*cost > 0 && n1_improved);

        // N2: stabilize_room
        bool n2_improved = false;
#if 0

        i = 0;
        do {
            j = 0;
            n2_improved = false;

            neighbourhood_stabilize_room_move best_stab_room_mv;
            neighbourhood_stabilize_room_result best_stab_room_result;
            double best_stab_room_tabu_cost = DBL_MAX;

            neighbourhood_stabilize_room_iter stab_room_iter;
            neighbourhood_stabilize_room_iter_init(&stab_room_iter, s);

            neighbourhood_stabilize_room_result stab_room_result;
            neighbourhood_stabilize_room_move stab_room_mv;

            while (neighbourhood_stabilize_room_iter_next(&stab_room_iter, &stab_room_mv)) {
                debug2("[%d.%d.%d] TS stabilize_room(c=%d r=%d)",
                    iter, i, j, stab_room_mv.c1, stab_room_mv.r2);

                neighbourhood_stabilize_room(s, &stab_room_mv,
                                  NEIGHBOURHOOD_PREDICT_ALWAYS,
                                  NEIGHBOURHOOD_PREDICT_ALWAYS,
                                  NEIGHBOURHOOD_PERFORM_NEVER,
                                  &stab_room_result);

                solution_fingerprint_t stab_room_fingerprint = *fingerprint;
                apply_solution_fingerprint_diff(&stab_room_fingerprint,
                                     stab_room_result.fingerprint_plus,
                                     stab_room_result.fingerprint_minus);

                debug2("[%d.stabilize_room.%d.%d] TS stabilize_room(c=%d r=%d) cost = %d -> {s=%llu, p=%llu}",
                    iter, i, j, stab_room_mv.c1, stab_room_mv.r2, swap_result.delta_cost,
                    stab_room_fingerprint.sum, stab_room_fingerprint.xor);

                double tabu_cost = tabu_list_stabilize_room_cost(tabu, stab_room_mv, stab_room_result, stab_room_fingerprint);
//                double tabu_cost = stab_room_result.delta_cost;

                if (!solution_fingerprint_equal(*fingerprint, stab_room_fingerprint) &&
                        tabu_cost < best_stab_room_tabu_cost /* || better than best sol */) {
                    debug("[%d.stabilize_room.%d.%d] TS: new best stab_room move candidate"
                        "(c=%d r=%d) of delta = %d (tabu cost = %g)",
                        iter, i, j,
                        stab_room_mv.c1, stab_room_mv.r2, stab_room_result.delta_cost,
                        tabu_cost);
                    best_stab_room_tabu_cost = tabu_cost;
                    best_stab_room_result = stab_room_result;
                    best_stab_room_mv = stab_room_mv;
                }

                neighbourhood_stabilize_room_iter_destroy(&stab_room_iter);

                j++;
            }

            if (stab_room_result.delta_cost < 0) {
                debug("[%d.%d] TS: using stabilize room move (c=%d r=%d) of cost %d",
                      iter, i, best_stab_room_mv.c1, best_stab_room_mv.r2, best_stab_room_result.delta_cost);
                neighbourhood_stabilize_room(s, &best_stab_room_mv,
                                             NEIGHBOURHOOD_PREDICT_ALWAYS,
                                             NEIGHBOURHOOD_PREDICT_ALWAYS,
                                             NEIGHBOURHOOD_PERFORM_ALWAYS,
                                             &stab_room_result);
                tabu_list_update_stabilize_room_stats(tabu, best_stab_room_mv, best_stab_room_result);
                apply_solution_fingerprint_diff(fingerprint,
                     stab_room_result.fingerprint_plus,
                     stab_room_result.fingerprint_minus);
                n2_improved = true;
                *cost += stab_room_result.delta_cost;
            }
        } while (*cost > 0 && n2_improved);

#endif
        improved = n1_improved || n2_improved;
    } while (*cost > 0 && improved);
}

static int tabu_search_destroy_local_minimum(
        int iter, solution *s, solution_fingerprint_t *fingerprint,
        int *cost, tabu_list *tabu, int power) {

    CRDSQT(s->model);

    neighbourhood_swap_move_info *swap_moves =
                mallocx(solution_assignment_count(s) * R * D * S, sizeof(neighbourhood_swap_move_info));
    int swap_move_cursor;

    int j = 0;
    for (int i = 0; i < power; i++) {
        debug("[%d] TS: destroying local minimum of cost =%d", iter, *cost);
        swap_move_cursor = 0;

        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, s);

        neighbourhood_swap_result swap_result;
        neighbourhood_swap_move swap_mv;


        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            neighbourhood_swap(s, &swap_mv,
                NEIGHBOURHOOD_PREDICT_ALWAYS,
                NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                NEIGHBOURHOOD_PERFORM_NEVER,
                &swap_result);

            if (swap_result.feasible) {
                solution_fingerprint_t swap_fingerprint = *fingerprint;
                apply_solution_fingerprint_diff(&swap_fingerprint,
                         swap_result.fingerprint_plus,
                         swap_result.fingerprint_minus);

                if (!solution_fingerprint_equal(*fingerprint, swap_fingerprint) &&
                    tabu_list_swap_move_is_allowed(tabu, swap_mv, iter)) {
                    neighbourhood_swap_move_info mv_res = {
                        .move = swap_mv,
                        .result = swap_result,
                        .fingerprint = swap_fingerprint,
                        .cost = neighbourhood_swap_move_cost(&swap_mv, s)
                    };

                    swap_moves[swap_move_cursor++] = mv_res;
                }
            }
        }

        qsort(swap_moves, swap_move_cursor, sizeof(neighbourhood_swap_move_info),
              neighbourhood_swap_move_result_compare);

//        render_solution_overview(s, "/tmp/x.png");

//        for (int ii = 0; ii < 100; ii++) {
//            const neighbourhood_swap_move_info *m = &swap_moves[ii];
//            debug("[%d] (c=%d (r=%d d=%d s=%d) c_2=%d (r=%d d=%d s=%d)) = mv_cost = %d, sol_delta_cost = %d",
//                  ii,
//                  m->move.c1, m->move.r1, m->move.d1, m->move.s1,
//                  m->move._c2, m->move.r2, m->move.d2, m->move.s2,
//                  m->cost, m->result.delta_cost);
//        }

//        do {
//            pick = (int) round(rand_triangular(0, (double) swap_move_cursor / 2, (double) swap_move_cursor / 2));
//        } while (pick < 0 || pick >= swap_move_cursor);
//        pick = i;

        if (swap_move_cursor == 0) {
            eprint("No destruction move available");
            exit(1);
        }
        int pick = 0;

        neighbourhood_swap_move_info picked_swap_mv = swap_moves[pick]; //...
        debug("[%d] TS: destruction move n. %d at index %d: (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> {s=%llu p=%llu}, "
                  "delta = %d", iter, i, pick,
              picked_swap_mv.move.c1, picked_swap_mv.move.r1, picked_swap_mv.move.d1, picked_swap_mv.move.s1,
              picked_swap_mv.move.r2, picked_swap_mv.move.d2, picked_swap_mv.move.s2,
              picked_swap_mv.fingerprint.sum, picked_swap_mv.fingerprint.xor,
              picked_swap_mv.result.delta_cost);

        neighbourhood_swap(s, &picked_swap_mv.move,
            NEIGHBOURHOOD_PREDICT_NEVER,
            NEIGHBOURHOOD_PREDICT_ALWAYS,
            NEIGHBOURHOOD_PREDICT_ALWAYS,
            NEIGHBOURHOOD_PERFORM_ALWAYS,
            &swap_result);
        apply_solution_fingerprint_diff(fingerprint,
                                        swap_result.fingerprint_plus,
                                        swap_result.fingerprint_minus);
        tabu_list_insert_swap_move(tabu, picked_swap_mv.move, iter);
        *cost += swap_result.delta_cost;
    }
}

bool tabu_search_solver_solve(tabu_search_solver *solver,
                              tabu_search_solver_config *config,
                              solution *sol_out) {
    CRDSQT(sol_out->model);
    const model *model = sol_out->model;

    long starting_time = ms();
    long time_limit = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    long iter_limit = config->iter_limit > 0 ? config->iter_limit : 0;

    tabu_search_solver_reinit(solver);

    solution s;
    solution_init(&s, model);

    // Find initial solution
    if (config->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(&s, config->starting_solution);
    }
    else {
        debug("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;
        feasible_solution_finder_find(&finder, &finder_config, &s);
    }

    // Compute initial solution's cost
    int room_capacity_cost = solution_room_capacity_cost(&s);
    int min_working_days_cost = solution_min_working_days_cost(&s);
    int curriculum_compactness_cost = solution_curriculum_compactness_cost(&s);
    int room_stability_cost = solution_room_stability_cost(&s);
    int cost =  room_capacity_cost + min_working_days_cost +
            curriculum_compactness_cost + room_stability_cost;
    solution_fingerprint_t fingerprint = solution_fingerprint(&s);

    // Mark initial solution as the best solution
    int best_sol_cost;
    solution best_sol;
    solution_init(&best_sol, model);

    best_sol_cost = cost;
    solution_copy(&best_sol, &s);

    // Create tabu list and put initial solution
    tabu_list tabu;
    tabu_list_init(&tabu, config->tabu_size, model);

    fileclear("/tmp/trend.txt");
    fileclear("/tmp/moves.txt");
    fileclear("/tmp/solutions.txt");

    char tmp[92];

    int iter = 0;
    do {
        if (iter_limit > 0 && !(iter < iter_limit)) {
            debug("Iter limit reached (%d), stopping here", iter);
            break;
        }
        if (time_limit > 0 && !(ms() < time_limit)) {
            debug("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }
//        render_solution_overview(&s, strmake("/tmp/sol-iter-%d.png", iter));

        if (iter > 50) {
            mssleep(0);
        }

        debug("Inserting solution into tabu list {s=%llu, p=%llu}", fingerprint.sum, fingerprint.xor);
        tabu_list_insert_solution(&tabu, fingerprint);
        tabu_search_solve_phase(iter, &s, &fingerprint, &cost, &tabu);

        // Insert current solution into tabu list
//
//        debug("[%d] TS: best swap move (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)), delta = %d"
//              " (tabu cost = %g)", iter,
//              best_swap_mv.c1, best_swap_mv.r1, best_swap_mv.d1, best_swap_mv.s1,
//              best_swap_mv.r2, best_swap_mv.d2, best_swap_mv.s2,
//              best_swap_result.delta_cost, best_swap_tabu_cost);
//
//        debug("[%d] TS: best stab_room move (c=%d r=%d), delta = %d"
//              " (tabu cost = %g)", iter,
//             best_stab_room_mv.c1, best_stab_room_mv.r2,
//             best_stab_room_result.delta_cost, best_stab_room_tabu_cost);
//
//        // Actually compute the new best solution and move to it
//        if (best_swap_tabu_cost <= best_stab_room_tabu_cost) {
//            // Perform N1:swap
//            debug("[%d] TS: using swap move", iter);
//
//            neighbourhood_swap(&s, &best_swap_mv,
//                           NEIGHBOURHOOD_PREDICT_NEVER,
//                           NEIGHBOURHOOD_PREDICT_ALWAYS,
//                           NEIGHBOURHOOD_PREDICT_ALWAYS,
//                           NEIGHBOURHOOD_PERFORM_ALWAYS,
//                           &swap_result);
//
//            room_capacity_cost += swap_result.delta_cost_room_capacity;
//            min_working_days_cost += swap_result.delta_cost_min_working_days;
//            curriculum_compactness_cost += swap_result.delta_cost_curriculum_compactness;
//            room_stability_cost += swap_result.delta_cost_room_stability;
//            cost += swap_result.delta_cost;
//            apply_solution_fingerprint_diff(&fingerprint,
//                                    swap_result.fingerprint_plus,
//                                    swap_result.fingerprint_minus);
//            // Update tabu stats
//            tabu_list_update_swap_stats(&tabu, best_swap_mv, best_swap_result);
//
////            filewrite("/tmp/moves.txt", true, "swap\n");
//        }
//        else {
//#if N2
//            // Perform N2:stabilize_room
//            debug("[%d] TS: using stabilize room move", iter);
//            neighbourhood_stabilize_room(&s, &best_stab_room_mv,
//                NEIGHBOURHOOD_PREDICT_ALWAYS,
//                NEIGHBOURHOOD_PREDICT_ALWAYS,
//                NEIGHBOURHOOD_PERFORM_ALWAYS,
//                &stab_room_result);
//
//            room_capacity_cost += stab_room_result.delta_cost_room_capacity;
//            min_working_days_cost += stab_room_result.delta_cost_min_working_days;
//            curriculum_compactness_cost += stab_room_result.delta_cost_curriculum_compactness;
//            room_stability_cost += stab_room_result.delta_cost_room_stability;
//            cost += stab_room_result.delta_cost;
//            apply_solution_fingerprint_diff(&fingerprint,
//                                    stab_room_result.fingerprint_plus,
//                                    stab_room_result.fingerprint_minus);
//            // Update tabu stats
//            tabu_list_update_stabilize_room_stats(&tabu, best_stab_room_mv, best_stab_room_result);
//
//            filewrite("/tmp/moves.txt", true, "stab\n");
//#endif
//        }

        assert(cost == solution_cost(&s));
        assert(fingerprint.sum == solution_fingerprint(&s).sum);
        assert(fingerprint.xor == solution_fingerprint(&s).xor);

        debug("[%d] TS: moved to solution {s=%llu, p=%llu} (rc=%d, mwd=%d, cc=%d, rs=%d) "
              "of cost = %d (best is %d)",
              iter, fingerprint.sum, fingerprint.xor,
              room_capacity_cost, min_working_days_cost,
              curriculum_compactness_cost, room_stability_cost,
              cost,
              best_sol_cost);

        snprintf(tmp, 92, "%d\n", cost);
        fileappend("/tmp/trend.txt", tmp);

        snprintf(tmp, 92, "sum=%llu xor=%llu\n", fingerprint.sum, fingerprint.xor);
        fileappend("/tmp/solutions.txt", tmp);

        if (cost < best_sol_cost) {
            verbose("[%d] TS: new best solution found, cost = %d (delta = %d)", iter, cost, cost - best_sol_cost);
            solution_copy(&best_sol, &s);
            best_sol_cost = cost;
        }
        else {
            debug("[%d] LS minimum reached", iter);
//            static const int DESTRUCTION_POWER = 3;
//            tabu_search_destroy_local_minimum(iter, &s, &fingerprint, &cost, &tabu, DESTRUCTION_POWER);
            debug("[%d] After destruction = %d", iter, cost);
        }


        iter++;
    } while (cost > 0);

    solution_destroy(&s);
    tabu_list_destroy(&tabu);

    if (best_sol_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    solution_copy(sol_out, &best_sol);

    return strempty(solver->error);
}
#endif