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
    solution_fingerprint_t *banned_solutions;
    GHashTable *moves_stats;
    size_t len;
    size_t index;
} tabu_list;

typedef struct tabu_list_move_stat_key {
    const tabu_list *tabu;
    neighbourhood_swap_move move;
} tabu_list_move_stat_key;

typedef struct tabu_list_move_stat_value {
    int count;
    int delta_sum;
} tabu_list_move_stat_value;

static guint tabu_list_move_stat_hash(gconstpointer  key) {
    const tabu_list_move_stat_key *stat_key = (tabu_list_move_stat_key *) key;

    uint i1 = INDEX4(stat_key->move.c1, move_stat->tabu->model->n_courses,
                  stat_key->move.r1, stat_key->tabu->model->n_rooms,
                  stat_key->move.d1, stat_key->tabu->model->n_days,
                  stat_key->move.s1, stat_key->tabu->model->n_slots);
    uint i2 = INDEX4(stat_key->move._c2, move_stat->tabu->model->n_courses,
                  stat_key->move.r2, stat_key->tabu->model->n_rooms,
                  stat_key->move.d2, stat_key->tabu->model->n_days,
                  stat_key->move.s2, stat_key->tabu->model->n_slots);
    return i1 + i2;
}

static gboolean tabu_list_move_stat_equal(gconstpointer  a, gconstpointer  b) {
    const tabu_list_move_stat_key *move_stat_a = (tabu_list_move_stat_key *) a;
    const tabu_list_move_stat_key *move_stat_b = (tabu_list_move_stat_key *) b;

    return
        (move_stat_a->move.c1 == move_stat_b->move.c1 &&
        move_stat_a->move.r1 == move_stat_b->move.r1 &&
        move_stat_a->move.d1 == move_stat_b->move.d1 &&
        move_stat_a->move.s1 == move_stat_b->move.s1 &&
        move_stat_a->move._c2 == move_stat_b->move._c2 &&
        move_stat_a->move.r2 == move_stat_b->move.r2 &&
        move_stat_a->move.d2 == move_stat_b->move.d2 &&
        move_stat_a->move.s2 == move_stat_b->move.s2) ||
        (move_stat_a->move.c1 == move_stat_b->move._c2 &&
        move_stat_a->move.r1 == move_stat_b->move.r2 &&
        move_stat_a->move.d1 == move_stat_b->move.d2 &&
        move_stat_a->move.s1 == move_stat_b->move.s2 &&
        move_stat_a->move._c2 == move_stat_b->move.c1 &&
        move_stat_a->move.r2 == move_stat_b->move.r1 &&
        move_stat_a->move.d2 == move_stat_b->move.d1 &&
        move_stat_a->move.s2 == move_stat_b->move.s1);
}

static void tabu_list_init(tabu_list *tabu, size_t tabu_size, const model *model) {
    debug("Initializing tabu list of size %lu", tabu_size);
    tabu->banned_solutions = callocx(tabu_size, sizeof(solution_fingerprint_t));
    tabu->len = tabu_size;
    tabu->index = 0;
    tabu->moves_stats = g_hash_table_new_full(tabu_list_move_stat_hash, tabu_list_move_stat_equal, free, free);
    tabu->model = model;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->banned_solutions);
    g_hash_table_destroy(tabu->moves_stats);
}

static bool tabu_list_solution_is_allowed(const tabu_list *tabu, solution_fingerprint_t f) {
    for (int i = 0; i < tabu->len; i++) {
        if (solution_fingerprint_equal(f, tabu->banned_solutions[i])) {
            debug2("TABU forbids {s=%llu, p=%llu}", f.sum, f.xor);
            return false;
        }
    }
    return true;
}

static double tabu_list_cost(const tabu_list *tabu, neighbourhood_swap_move move,
                             neighbourhood_swap_result result, solution_fingerprint_t fingerprint) {
    if (!tabu_list_solution_is_allowed(tabu, fingerprint))
        return DBL_MAX;

    tabu_list_move_stat_key k;
    k.tabu = tabu;
    k.move = move;

    void * v = g_hash_table_lookup(tabu->moves_stats, &k);
    static double D = 20;
    double penalty = 0;
    if (v) {
        tabu_list_move_stat_value *vv = (tabu_list_move_stat_value *) v;
        double delta_mean = (double) (vv->delta_sum + vv->count) / (vv->count);
        penalty = delta_mean;
        debug("tabu_list_cost (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> [count=%d, delta_mean=%g] -> %g",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          vv->count, (double) vv->delta_sum / vv->count, result.delta_cost + D * penalty);
    }

    debug2("tabu_list_cost (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> %g",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,  result.delta_cost + D * penalty);

    return result.delta_cost + D * penalty;
}


static void tabu_list_update_stats(tabu_list* tabu,
                                   neighbourhood_swap_move move,
                                   neighbourhood_swap_result result) {
    tabu_list_move_stat_key *stat_key = mallocx(1, sizeof(neighbourhood_swap_move));
    neighbourhood_swap_move_copy(&stat_key->move, &move);
    stat_key->tabu = tabu;

    tabu_list_move_stat_value * stat_val;
    void *v = g_hash_table_lookup(tabu->moves_stats, stat_key);
    if (v) {
        stat_val = (tabu_list_move_stat_value *) v;
        free(stat_key);
    }
    else {
        stat_val = mallocx(1, sizeof(tabu_list_move_stat_value));
        stat_val->count = 0;
        g_hash_table_insert(tabu->moves_stats, stat_key, stat_val);
    }

    stat_val->count++;
    stat_val->delta_sum += result.delta_cost;

    debug("tabu_list_update_stats (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> [count=%d, delta_mean=%g]",
          move.c1, move.r1, move.d1, move.s1, move.r2, move.d2, move.s2,
          stat_val->count, (double) stat_val->delta_sum / stat_val->count);
}

//static bool tabu_list_score(const tabu_list *tabu, solution *starting_solution, ne) {
//    for (int i = 0; i < tabu->len; i++) {
//        if (solution_tabu_fingerprint_equal(f, tabu->banned_solutions[i])) {
//            return false;
//        }
//    }
//    return true;
//}

static void tabu_list_insert_solution(tabu_list *tabu, solution_fingerprint_t f) {
//    debug("tabu_list_insert [%lu]=(rc=%d, mwd=%d, cc=%d, rs=%d)",
//       tabu->index,
//       f.room_capacity_cost, f.min_working_days_cost,
//       f.curriculum_compactness_cost, f.room_stability_cost);
    debug2("tabu_list_insert_solution [%lu]=(s=%llu, p=%llu)", tabu->index, f.sum, f.xor);

#ifdef DEBUG
    for (int i = 0; i < tabu->len; i++) {
        solution_fingerprint_t *sf = &tabu->banned_solutions[i];
        debug2("tabu_list[%d]=(s=%llu, p=%llu) %s", i, sf->sum, sf->xor, i == tabu->index ? "   *" : "");
        assert(!solution_fingerprint_equal(*sf, f));
    }
#endif

    tabu->banned_solutions[tabu->index] = f;
    tabu->index = (tabu->index + 1) % tabu->len;
}

void tabu_search_solver_config_init(tabu_search_solver_config *config) {
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
    config->tabu_size = 14;
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

typedef struct neighbourhood_swap_move_result {
    neighbourhood_swap_move move;
    neighbourhood_swap_result result;
    solution_fingerprint_t fingerprint;
} neighbourhood_swap_move_result;


int neighbourhood_swap_move_result_compare_data(const void *_1, const void *_2, void *data) {
    neighbourhood_swap_move_result *mv_res1 = (neighbourhood_swap_move_result *) _1;
    neighbourhood_swap_move_result *mv_res2 = (neighbourhood_swap_move_result *) _2;
    tabu_list *tabu = data;

    return mv_res1->result.delta_cost - mv_res2->result.delta_cost;
}

int neighbourhood_swap_move_result_compare(const void *_1, const void *_2) {
    neighbourhood_swap_move_result *mv_res1 = (neighbourhood_swap_move_result *) _1;
    neighbourhood_swap_move_result *mv_res2 = (neighbourhood_swap_move_result *) _2;

    return mv_res1->result.delta_cost - mv_res2->result.delta_cost;
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
bool tabu_search_solver_solve(tabu_search_solver *solver,
                              tabu_search_solver_config *config,
                              solution *sol_out) {
    CRDSQT(sol_out->model);
    const model *model = sol_out->model;

    long starting_time = ms();
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
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

    filewrite("/tmp/trend.txt", false, "");

    int iter = 0;
    do {
        if (iter_limit > 0 && !(iter < iter_limit)) {
            debug("Iter limit reached (%d), stopping here", iter);
            break;
        }
        if (deadline > 0 && !(ms() < deadline)) {
            debug("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }
//        render_solution_overview(&s, strmake("/tmp/sol-iter-%d.png", iter));

        if (iter > 47)
#if DEBUG
            mssleep(400);
#else
        ;
#endif
        int j = 0;
        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, &s);

        neighbourhood_swap_result swap_result;
        neighbourhood_swap_move swap_mv;

        neighbourhood_swap_move best_swap_move;
        neighbourhood_swap_result best_swap_result;
        double best_swap_tabu_cost = DBL_MAX;

        debug("[%d] TS: starting fingerprint = {s=%llu, p=%llu)",
              iter, fingerprint.sum, fingerprint.xor);

        // N1: swap
        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   iter, j,
                   swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                   swap_mv.r2, swap_mv.d2, swap_mv.s2);

            neighbourhood_swap(&s, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.feasible) {
                solution_fingerprint_t swap_fingerprint = fingerprint;
                apply_solution_fingerprint_diff(&swap_fingerprint,
                                                swap_result.fingerprint_plus,
                                                swap_result.fingerprint_minus);

                debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) cost = %d -> {s=%llu, p=%llu}",
                   iter, j,
                   swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                   swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost,
                   swap_fingerprint.sum, swap_fingerprint.xor);

                double tabu_cost = tabu_list_cost(&tabu, swap_mv, swap_result, swap_fingerprint);

                if (!solution_fingerprint_equal(fingerprint, swap_fingerprint) &&
                    tabu_cost < best_swap_tabu_cost /* || better than best sol */) {
                    debug("[%d.%d] TS: new best swap move candidate swap"
                          "(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) of delta = %d (tabu cost = %g)",
                          iter, j,
                          swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                          swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost,
                          tabu_cost);
                    best_swap_tabu_cost = tabu_cost;
                    best_swap_result = swap_result;
                    best_swap_move = swap_mv;
                } else {
                    if (solution_fingerprint_equal(fingerprint, swap_fingerprint)) {
//                        render_solution_overview(&s, "/tmp/x.png");
                        debug2("[%d.%d] TS: swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)), "
                              "has the same fingerprint of the current solution",
                              iter, j, swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                              swap_mv.r2, swap_mv.d2, swap_mv.s2);
                    }
                }
            }

            j++;
        }

        // Insert current solution into tabu list
        tabu_list_insert_solution(&tabu, fingerprint);

        // Update move stats
        tabu_list_update_stats(&tabu, best_swap_move, best_swap_result);

        debug("[%d] TS: best move (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)), delta = %d"
              " (tabu cost = %g)", iter,
              best_swap_move.c1, best_swap_move.r1, best_swap_move.d1, best_swap_move.s1,
              best_swap_move.r2, best_swap_move.d2, best_swap_move.s2,
              best_swap_result.delta_cost, best_swap_tabu_cost);

        /*
            neighbourhood_stabilize_room_iter stabilize_room_iter;
            neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, &s);

            neighbourhood_stabilize_room_result stabilize_room_result;
            neighbourhood_stabilize_room_move stabilize_room_mv;

            neighbourhood_stabilize_room_move stabilize_room_best_mv;
            int stabilize_room_best_delta = INT_MAX;


            while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter,
                                                          &stabilize_room_mv)) {
                debug2("[%d.%d] TS stabilize_room(c=%d r=%d)",
                       iter, j, stabilize_room_mv.c1, stabilize_room_mv.r2);

                // TODO: avoid to reuse tabu's solution (implement stab_room_back)

                neighbourhood_stabilize_room(&s, &stabilize_room_mv,
                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                   NEIGHBOURHOOD_PERFORM_NEVER,
                   &stabilize_room_result);


                solution_fingerprint_t stabilize_room_fingerprint =
                        s_fingerprint + stabilize_room_result.fingerprint_diff;

                if (stabilize_room_result.delta_cost < stabilize_room_best_delta &&
                     !tabu_list_solution_is_allowed(&tabu, stabilize_room_fingerprint)) {
                    stabilize_room_best_delta = stabilize_room_result.delta_cost;
                    stabilize_room_best_mv = stabilize_room_mv;
                    debug("New best stab_room candidate %d %d of cost %d",
                          stabilize_room_mv.c1, stabilize_room_mv.r2, stabilize_room_result.delta_cost);
                }
            }

            debug("[%d] TS: stabilize_room best move (c=%d, r=%d) has delta cost = %d",
                  iter, stabilize_room_best_mv.c1, stabilize_room_best_mv.r2,
                  stabilize_room_best_delta);
//            snprintf(tmp, 256, "/tmp/overview.before-best-move-%d.png", iter);
//            render_solution_overview(&s, tmp);
            debug("[%d] TS: non negative move, trying stabilize_room neighbourhood", iter);
            if (stabilize_room_best_delta < 0) {
                // Move to the new solution since is better

                solution_fingerprint_t ff1 = solution_fingerprint(&s);

                neighbourhood_stabilize_room(&s, &stabilize_room_best_mv,
                    NEIGHBOURHOOD_PREDICT_ALWAYS,
                    NEIGHBOURHOOD_PREDICT_ALWAYS, // TODO: compute fprint
                    NEIGHBOURHOOD_PERFORM_ALWAYS,
                    &stabilize_room_result);

                solution_fingerprint_t ff2 = solution_fingerprint(&s);

                if (ff1 + stabilize_room_result.fingerprint_diff != ff2) {
                    eprint("wrong fprint after stab_room, expected = %llu, found %llu + %llu", ff2, ff1, stabilize_room_result.fingerprint_diff);
                    exit(82);
                }

                room_capacity_cost += stabilize_room_result.delta_cost_room_capacity;
                min_working_days_cost += stabilize_room_result.delta_cost_min_working_days;
                curriculum_compactness_cost += stabilize_room_result.delta_cost_curriculum_compactness;
                room_stability_cost += stabilize_room_result.delta_cost_room_stability;
                debug("stabilize_room, cost was %d", cost);
                cost += stabilize_room_result.delta_cost;
                debug("stabilize_room, is %d (delta = %d)", cost, stabilize_room_result.delta_cost);

                stabilize_improvement = true;
                stabilize_room_improvements++;
            } else {
                debug("[%d] TS: doing nothing since best stabilize_room has cost = %d",
                      iter, stabilize_room_best_delta);
            }
        }
*/
        // Actually compute the new best solution and move to it
        neighbourhood_swap(&s, &best_swap_move,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PERFORM_ALWAYS,
                           &swap_result);

        room_capacity_cost += swap_result.delta_cost_room_capacity;
        min_working_days_cost += swap_result.delta_cost_min_working_days;
        curriculum_compactness_cost += swap_result.delta_cost_curriculum_compactness;
        room_stability_cost += swap_result.delta_cost_room_stability;
        cost += swap_result.delta_cost;
        apply_solution_fingerprint_diff(&fingerprint,
                                swap_result.fingerprint_plus,
                                swap_result.fingerprint_minus);

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

        filewrite("/tmp/trend.txt", true, strmake("%d\n", cost));


        if (cost < best_sol_cost) {
            verbose("[%d] TS: new best solution found, cost = %d (delta = %d)", iter, cost, cost - best_sol_cost);
            solution_copy(&best_sol, &s);
            best_sol_cost = cost;
        } else {
//            print("LS minimum reached");
//            break;
        }

    neighbourhood_swap_iter_destroy(&swap_iter);
    iter++;

    } while (cost > 0);


    solution_destroy(&s);
    tabu_list_destroy(&tabu);

    if (best_sol_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    solution_copy(sol_out, &best_sol);

    return strempty(solver->error);
}
#else
bool tabu_search_solver_solve(tabu_search_solver *solver,
                              tabu_search_solver_config *config,
                              solution *sol_out) {
    CRDSQT(sol_out->model);
    const model *model = sol_out->model;
    tabu_search_solver_reinit(solver);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);
    finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

    long starting_time = ms();
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    long iter_limit = config->iter_limit > 0 ? config->iter_limit : 0;
    solution s;
    solution_init(&s, model);

    // Find initial solution
    if (config->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(&s, config->starting_solution);
    }
    else {
        debug("Finding initial feasible solution...");
        feasible_solution_finder_find(&finder, &finder_config, &s);
    }

    neighbourhood_swap_move_result *swap_moves =
            mallocx(solution_assignment_count(&s) * R * D * S, sizeof(neighbourhood_swap_move_result));
    double swap_move_pick_randomizer = 0;

    // Compute initial solution's cost
    int room_capacity_cost = solution_room_capacity_cost(&s);
    int min_working_days_cost = solution_min_working_days_cost(&s);
    int curriculum_compactness_cost = solution_curriculum_compactness_cost(&s);
    int room_stability_cost = solution_room_stability_cost(&s);
    int cost =  room_capacity_cost + min_working_days_cost +
            curriculum_compactness_cost + room_stability_cost;

    // Mark initial solution as the best solution
    int best_sol_cost;
    solution best_sol;
    solution_init(&best_sol, model);

    best_sol_cost = cost;
    solution_copy(&best_sol, &s);

    // Create tabu list and put initial solution
    tabu_list tabu;
    tabu_list_init(&tabu, config->tabu_size);

    int swap_improvements = 0;
    int stabilize_room_improvements = 0;

    int iter = 0;
    bool slow = false;
    do {
        if (slow)
            mssleep(1);
        if (iter_limit > 0 && !(iter < iter_limit)) {
            debug("Iter limit reached (%d), stopping here", iter);
            break;
        }
        if (deadline > 0 && !(ms() < deadline)) {
            debug("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }

        int j = 0;
        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, &s);

        neighbourhood_swap_result swap_result;
        neighbourhood_swap_move swap_mv;
//        neighbourhood_swap_move best_moves[1000];
//        int best_move_count = 0;

        int best_move_delta = INT_MAX;
        int swap_move_cursor = 0;

        solution_fingerprint_t s_fingerprint = solution_fingerprint(&s);
        debug("[%d] TS: starting fingeprint = %llu", iter, s_fingerprint);

        // N1: swap
        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   iter, j,
                   swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                   swap_mv.r2, swap_mv.d2, swap_mv.s2);

//            neighbourhood_swap(&s, &swap_mv,
//                               NEIGHBOURHOOD_PREDICT_ALWAYS,
//                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
//                               NEIGHBOURHOOD_PERFORM_NEVER,
//                               &swap_result);

            neighbourhood_swap(&s, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE,
                               &swap_result);

            if (swap_result.feasible) {
                debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) is feasible, delta_cost = %d",
                   iter, j,
                   swap_mv.c1, swap_mv.r1, swap_mv.d1, swap_mv.s1,
                   swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost);

//                solution_tabu_fingerprint f = {
//                    .room_capacity_cost = room_capacity_cost + swap_result.delta_cost_room_capacity,
//                    .min_working_days_cost = min_working_days_cost + swap_result.delta_cost_min_working_days,
//                    .curriculum_compactness_cost = curriculum_compactness_cost + swap_result.delta_cost_curriculum_compactness,
//                    .room_stability_cost = room_stability_cost + swap_result.delta_cost_room_stability
//                };
                // Save the move and the result
//                solution_tabu_fingerprint f = f +
//                        (mv.c1 + mv._c2) * 9391 + (mv.r1 + mv.r2) * 86453 +
//                        (mv.d1 + mv.d2) * 5273 + (mv.s1 + mv.s2) * 34147;

//                solution_tabu_fingerprint swap_fingerprint = solution_fingerprint(&s);


                solution_fingerprint_t swap_fingerprint = s_fingerprint + swap_result.fingerprint_diff;
//#if DEBUG
//                solution_fingerprint_t  fff = solution_fingerprint(&s);
//                if (swap_fingerprint != fff) {
//                    debug("Wrong fingerprint expected=%llu != found=%llu (s_fingerprint=%llu, swap_result.fingerprint_diff=%llu) ",
//                          swap_fingerprint, fff, s_fingerprint, swap_result.fingerprint_diff);
//                    exit(1);
//                } else {
//                    debug("Fingeprint OK %llu = %llu", swap_fingerprint, fff);
//                }
//#endif
                if (swap_fingerprint != s_fingerprint &&
                    !tabu_list_contains(&tabu, swap_fingerprint)) {
                    neighbourhood_swap_move_result mv_res = {
                        swap_mv,
                        swap_result,
                        swap_fingerprint
                    };
                    swap_moves[swap_move_cursor++] = mv_res;
                }


//                if (swap_result.delta_cost <= best_move_delta && f != s_fingerprint && !tabu_list_contains(&tabu, f)) {
//                    debug("[%d.%d] TS: new candidate for best neighbourhood move %llu (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)), "
//                          "delta = %d", iter, j, f, mv.c1, mv.r1, mv.d1, mv.s1, mv.r2, mv.d2, mv.s2, swap_result.delta_cost);
//                    if (swap_result.delta_cost < best_move_delta) {
//                        best_move_count = 0;
//                        best_move_delta = swap_result.delta_cost;
//                    }
//                    if (best_move_count < 1000)
//                        best_moves[best_move_count++] = mv;
//                }

                neighbourhood_swap_back(&s, &swap_mv, &swap_result);

//#if DEBUG
//                solution_fingerprint_t  fff2 = solution_fingerprint(&s);
//                if (s_fingerprint != fff2) {
//                    debug("Wrong fingerprint after swap back expected=%llu != found=%llu",
//                          s_fingerprint, fff2);
//                    exit(1);
//                } else {
//                    debug("Fingeprint OK %llu = %llu", s_fingerprint, fff2);
//                }
//#endif
            }

            j++;
        }

        if (swap_move_cursor == 0) {
            verbose("Tabu search can't improve further, stopping here");
            break;
        }

        qsort(swap_moves, swap_move_cursor, sizeof(neighbourhood_swap_move_result),
              neighbourhood_swap_move_result_compare);
//
//        for (int i = 0; i < swap_move_cursor; i++) {
//            debug2("move[%d]=(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) delta cost = %d -> {%llu}", i,
//                  swap_moves[i].move.c1, swap_moves[i].move.r1, swap_moves[i].move.d1, swap_moves[i].move.s1,
//                  swap_moves[i].move.r2, swap_moves[i].move.d2, swap_moves[i].move.s2, swap_moves[i].result.delta_cost,
//                  swap_moves[i].solution_fingerprint);
//        }

        tabu_list_insert(&tabu, s_fingerprint);

        // Pick a move at random among the best ones

        int pick;
        do {
            pick = (int) round(rand_triangular(0, swap_move_pick_randomizer, swap_move_pick_randomizer));
        } while (pick < 0 || pick >= swap_move_cursor);

        debug("[%d] TS: move pick = %d", iter, pick);

        int last_cost = cost;

        neighbourhood_swap_move_result picked_swap_mv = swap_moves[pick]; //...
        debug("[%d] TS: picked move [%d] (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> {%llu}, "
                  "delta = %d", iter, pick,
              picked_swap_mv.move.c1, picked_swap_mv.move.r1, picked_swap_mv.move.d1, picked_swap_mv.move.s1,
              picked_swap_mv.move.r2, picked_swap_mv.move.d2, picked_swap_mv.move.s2, picked_swap_mv.fingerprint,
              picked_swap_mv.result.delta_cost);

        bool stabilize_improvement = false;
        if (picked_swap_mv.result.delta_cost >= 0) {
            slow = true;
//            char tmp[256];
//            snprintf(tmp, 256, "/tmp/overview.before-%d.png", iter);
//            render_solution_overview(&s, tmp);
//            debug("[%d] TS: non negative move, trying stabilize_room neighbourhood", iter);

            neighbourhood_stabilize_room_iter stabilize_room_iter;
            neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, &s);

            neighbourhood_stabilize_room_result stabilize_room_result;
            neighbourhood_stabilize_room_move stabilize_room_mv;

            neighbourhood_stabilize_room_move stabilize_room_best_mv;
            int stabilize_room_best_delta = INT_MAX;


            while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter,
                                                          &stabilize_room_mv)) {
                debug2("[%d.%d] TS stabilize_room(c=%d r=%d)",
                       iter, j, stabilize_room_mv.c1, stabilize_room_mv.r2);

                // TODO: avoid to reuse tabu's solution (implement stab_room_back)

                neighbourhood_stabilize_room(&s, &stabilize_room_mv,
                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                   NEIGHBOURHOOD_PERFORM_NEVER,
                   &stabilize_room_result);


                solution_fingerprint_t stabilize_room_fingerprint =
                        s_fingerprint + stabilize_room_result.fingerprint_diff;

                if (stabilize_room_result.delta_cost < stabilize_room_best_delta &&
                     !tabu_list_contains(&tabu, stabilize_room_fingerprint)) {
                    stabilize_room_best_delta = stabilize_room_result.delta_cost;
                    stabilize_room_best_mv = stabilize_room_mv;
                    debug("New best stab_room candidate %d %d of cost %d",
                          stabilize_room_mv.c1, stabilize_room_mv.r2, stabilize_room_result.delta_cost);
                }
            }

            debug("[%d] TS: stabilize_room best move (c=%d, r=%d) has delta cost = %d",
                  iter, stabilize_room_best_mv.c1, stabilize_room_best_mv.r2,
                  stabilize_room_best_delta);
//            snprintf(tmp, 256, "/tmp/overview.before-best-move-%d.png", iter);
//            render_solution_overview(&s, tmp);
            debug("[%d] TS: non negative move, trying stabilize_room neighbourhood", iter);
            if (stabilize_room_best_delta < 0) {
                // Move to the new solution since is better

                solution_fingerprint_t ff1 = solution_fingerprint(&s);

                neighbourhood_stabilize_room(&s, &stabilize_room_best_mv,
                    NEIGHBOURHOOD_PREDICT_ALWAYS,
                    NEIGHBOURHOOD_PREDICT_ALWAYS, // TODO: compute fprint
                    NEIGHBOURHOOD_PERFORM_ALWAYS,
                    &stabilize_room_result);

                solution_fingerprint_t ff2 = solution_fingerprint(&s);

                if (ff1 + stabilize_room_result.fingerprint_diff != ff2) {
                    eprint("wrong fprint after stab_room, expected = %llu, found %llu + %llu", ff2, ff1, stabilize_room_result.fingerprint_diff);
                    exit(82);
                }

                room_capacity_cost += stabilize_room_result.delta_cost_room_capacity;
                min_working_days_cost += stabilize_room_result.delta_cost_min_working_days;
                curriculum_compactness_cost += stabilize_room_result.delta_cost_curriculum_compactness;
                room_stability_cost += stabilize_room_result.delta_cost_room_stability;
                debug("stabilize_room, cost was %d", cost);
                cost += stabilize_room_result.delta_cost;
                debug("stabilize_room, is %d (delta = %d)", cost, stabilize_room_result.delta_cost);

                stabilize_improvement = true;
                stabilize_room_improvements++;
            } else {
                debug("[%d] TS: doing nothing since best stabilize_room has cost = %d",
                      iter, stabilize_room_best_delta);
            }
        }

        if (!stabilize_improvement) {
            // Actually compute the new best solution and move to it
            neighbourhood_swap(&s, &picked_swap_mv.move,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_ALWAYS, // TODO: update fingerprint
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);

            room_capacity_cost += swap_result.delta_cost_room_capacity;
            min_working_days_cost += swap_result.delta_cost_min_working_days;
            curriculum_compactness_cost += swap_result.delta_cost_curriculum_compactness;
            room_stability_cost += swap_result.delta_cost_room_stability;
            cost += swap_result.delta_cost;

            if (swap_result.delta_cost < 0)
                swap_improvements++;
        }

        #if DEBUG
            if (cost != solution_cost(&s)) {
            debug("Expected %d, found %d", cost, solution_cost(&s));
                exit(13);
             }
        #endif
        debug("[%d] TS: moved to solution (rc=%d, mwd=%d, cc=%d, rs=%d) of cost = %d (best is %d)",
              iter,
              room_capacity_cost, min_working_days_cost,
              curriculum_compactness_cost, room_stability_cost,
              cost,
              best_sol_cost);

//        if (cost == last_cost) {
        if (cost == last_cost)
            swap_move_pick_randomizer = swap_move_pick_randomizer > 0 ? swap_move_pick_randomizer * 1.3  : 0.5;
        else
            swap_move_pick_randomizer = swap_move_pick_randomizer > 1 ? swap_move_pick_randomizer * 0.5 : 0;
        swap_move_pick_randomizer = RANGIFY(0, swap_move_pick_randomizer, swap_move_cursor - 1);

        debug("swap_move_pick_randomizer = %g", swap_move_pick_randomizer);

        if (cost < best_sol_cost) {
            verbose("[%d] TS: new best solution found, cost = %d (delta = %d)", iter, cost, cost - best_sol_cost);
            solution_copy(&best_sol, &s);
            best_sol_cost = cost;
        }

        neighbourhood_swap_iter_destroy(&swap_iter);
        iter++;
    } while (cost > 0);


    solution_destroy(&s);
    tabu_list_destroy(&tabu);

    if (best_sol_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    solution_copy(sol_out, &best_sol);

    debug("swap_improvements = %d", swap_improvements);
    debug("stabilize_room_improvements = %d", stabilize_room_improvements);

    return strempty(solver->error);
}
#endif