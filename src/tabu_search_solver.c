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
#include "neighbourhood.h"
#include "feasible_solution_finder.h"
#include "renderer.h"


//typedef struct solution_tabu_fingerprint {
//    int room_capacity_cost;
//    int min_working_days_cost;
//    int curriculum_compactness_cost;
//    int room_stability_cost;
//} solution_tabu_fingerprint;
typedef unsigned long long solution_tabu_fingerprint;

typedef struct tabu_list {
    solution_tabu_fingerprint *banned;
    size_t len;
    size_t index;
} tabu_list;


static bool solution_tabu_fingerprint_equal(solution_tabu_fingerprint f1,
                                            solution_tabu_fingerprint f2) {
    return f1 == f2;
//        f1.room_capacity_cost == f2.room_capacity_cost &&
//        f1.min_working_days_cost == f2.min_working_days_cost &&
//        f1.curriculum_compactness_cost == f2.curriculum_compactness_cost &&
//        f1.room_stability_cost == f2.room_stability_cost;
}

static void tabu_list_init(tabu_list *tabu, size_t tabu_size) {
    debug("Initializing tabu list of size %lu", tabu_size);
    tabu->banned = callocx(tabu_size, sizeof(solution_tabu_fingerprint));
    tabu->len = tabu_size;
    tabu->index = 0;
}

static void tabu_list_destroy(tabu_list *tabu) {
    free(tabu->banned);
}

static bool tabu_list_contains(const tabu_list *tabu, solution_tabu_fingerprint f) {
//    debug("tabu_list_contains (rc=%d, mwd=%d, cc=%d, rs=%d)",
//           f.room_capacity_cost, f.min_working_days_cost,
//           f.curriculum_compactness_cost, f.room_stability_cost);
    debug2("tabu_list_contains = %llu", f);
    for (int i = 0; i < tabu->len; i++) {
        if (solution_tabu_fingerprint_equal(f, tabu->banned[i])) {
            debug2("TRUE");
            return true;
        }
    }
    debug2("FALSE");
    return false;
}

static void tabu_list_insert(tabu_list *tabu, solution_tabu_fingerprint f) {
//    debug("tabu_list_insert [%lu]=(rc=%d, mwd=%d, cc=%d, rs=%d)",
//       tabu->index,
//       f.room_capacity_cost, f.min_working_days_cost,
//       f.curriculum_compactness_cost, f.room_stability_cost);
    debug2("tabu_list_insert [%lu]=%lu", tabu->index, f);

#ifdef DEBUG
    for (int i = 0; i < tabu->len; i++) {
        solution_tabu_fingerprint *sf = &tabu->banned[i];
//        debug("tabu_list[%d]=(rc=%d, mwd=%d, cc=%d, rs=%d)",
//           i,
//           sf->room_capacity_cost, sf->min_working_days_cost,
//           sf->curriculum_compactness_cost, sf->room_stability_cost);
        debug2("tabu_list[%d]=%llu%s", i, *sf, i == tabu->index ? "   *" : "");
        if (*sf == f) {
            eprint("Inserting duplicate solution already in tabu list");
            exit(5);
        }
    }
#endif

    tabu->banned[tabu->index] = f;
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
    solution_tabu_fingerprint solution_fingerprint;
} neighbourhood_swap_move_result;


int neighbourhood_swap_move_result_compare(const void *_1, const void *_2, void *data) {
    neighbourhood_swap_move_result *mv_res1 = (neighbourhood_swap_move_result *) _1;
    neighbourhood_swap_move_result *mv_res2 = (neighbourhood_swap_move_result *) _2;
    tabu_list *tabu = data;

    return mv_res1->result.delta_cost - mv_res2->result.delta_cost;
}

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
    debug("Initial solution found, cost = %d", cost);

    // Mark initial solution as the best solution
    int best_sol_cost;
    solution best_sol;
    solution_init(&best_sol, model);

    best_sol_cost = cost;
    solution_copy(&best_sol, &s);

    // Create tabu list and put initial solution
    tabu_list tabu;
    tabu_list_init(&tabu, config->tabu_size);

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

        int j = 0;

        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, &s);

        neighbourhood_swap_result swap_result;
        neighbourhood_swap_move mv;

//        neighbourhood_swap_move best_moves[1000];
//        int best_move_count = 0;

        int best_move_delta = INT_MAX;
        int swap_move_cursor = 0;

        solution_tabu_fingerprint s_fingerprint = solution_fingerprint(&s);

        // Inspect all the neighbourhood
        while (neighbourhood_swap_iter_next(&swap_iter, &mv)) {
            debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   iter, j, mv.c1, mv.r1, mv.d1, mv.s1, mv.r2, mv.d2, mv.s2);


//            neighbourhood_swap(&s, &mv,
//                               NEIGHBOURHOOD_PREDICT_ALWAYS,
//                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
//                               NEIGHBOURHOOD_PERFORM_NEVER,
//                               &swap_result);

            neighbourhood_swap(&s, &mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE,
                               &swap_result);

            if (swap_result.feasible) {
                debug2("[%d.%d] TS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) is feasible, delta_cost = %d",
                   iter, j, mv.c1, mv.r1, mv.d1, mv.s1, mv.r2, mv.d2, mv.s2, swap_result.delta_cost);

//                solution_tabu_fingerprint f = {
//                    .room_capacity_cost = room_capacity_cost + swap_result.delta_cost_room_capacity,
//                    .min_working_days_cost = min_working_days_cost + swap_result.delta_cost_min_working_days,
//                    .curriculum_compactness_cost = curriculum_compactness_cost + swap_result.delta_cost_curriculum_compactness,
//                    .room_stability_cost = room_stability_cost + swap_result.delta_cost_room_stability
//                };
                // Save the move and the result
//                solution_tabu_fingerprint f = solution_fingerprint(&s);
                solution_tabu_fingerprint f = f +
                        mv.c1 * 9391 +
                        mv.r1 * 877 + mv.d1 * 86453 + mv.s1 * 2729 +
                        mv.r2 * 5273 + mv.d2 * 8467 + mv.s2 * 34147;
                if (f != s_fingerprint && !tabu_list_contains(&tabu, f)) {
                    neighbourhood_swap_move_result mv_res = {
                        mv,
                        swap_result,
                        f
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

                neighbourhood_swap_back(&s, &mv);
            }

            j++;
        }

        if (swap_move_cursor == 0) {
            verbose("Tabu search can't improve further, stopping here");
            break;
        }
        g_qsort_with_data(swap_moves, swap_move_cursor, sizeof(neighbourhood_swap_move_result),
              neighbourhood_swap_move_result_compare, &tabu);
//        qsort
        for (int i = 0; i < swap_move_cursor; i++) {
            debug2("move[%d]=(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) delta cost = %d -> {%llu}", i,
                  swap_moves[i].move.c1, swap_moves[i].move.r1, swap_moves[i].move.d1, swap_moves[i].move.s1,
                  swap_moves[i].move.r2, swap_moves[i].move.d2, swap_moves[i].move.s2, swap_moves[i].result.delta_cost,
                  swap_moves[i].solution_fingerprint);
        }
//        if (best_move_delta == INT_MAX) {
//            verbose("Tabu search can't improve further, stopping here");
//            break;
//        }
        // Insert the previous solution into the tabu list
//        solution_tabu_fingerprint f = {
//            .room_capacity_cost = room_capacity_cost,
//            .min_working_days_cost = min_working_days_cost,
//            .curriculum_compactness_cost = curriculum_compactness_cost,
//            .room_stability_cost = room_stability_cost
//        };

        tabu_list_insert(&tabu, s_fingerprint);

        // Pick a move at random among the best ones
//        int r = rand_int_range(0, best_move_count);
//        neighbourhood_swap_move best_mv = best_moves[r];


        int pick;
        do {
            pick = ABS(round(rand_normal(0, swap_move_pick_randomizer)));
        } while (pick < 0 || pick >= swap_move_cursor);
        debug("[%d] TS: move pick = %d", iter, pick);

        int last_cost = cost;

        neighbourhood_swap_move_result the_move = swap_moves[pick]; //...
        debug("[%d] TS: picked move [%d] (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> {%llu}, "
                  "delta = %d", iter, pick,
                  the_move.move.c1, the_move.move.r1, the_move.move.d1, the_move.move.s1,
                  the_move.move.r2, the_move.move.d2, the_move.move.s2, the_move.solution_fingerprint,
                  the_move.result.delta_cost);
        // Actually compute the new best solution and move to it
        neighbourhood_swap(&s, &the_move.move,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PERFORM_ALWAYS,
                           &swap_result);
        solution_invalidate(&s); // recompute solution_helper

        room_capacity_cost = room_capacity_cost + swap_result.delta_cost_room_capacity;
        min_working_days_cost = min_working_days_cost + swap_result.delta_cost_min_working_days;
        curriculum_compactness_cost = curriculum_compactness_cost + swap_result.delta_cost_curriculum_compactness;
        room_stability_cost = room_stability_cost + swap_result.delta_cost_room_stability;
#if DEBUG
        if (cost + swap_result.delta_cost != room_capacity_cost + min_working_days_cost +
                curriculum_compactness_cost + room_stability_cost) {
            eprint("Expected same cost, found %d != %d", cost, cost + swap_result.delta_cost);
            exit(14);
        }
        if (swap_result.delta_cost != the_move.result.delta_cost) {
            eprint("Expected same cost, found %d != %d", swap_result.delta_cost, the_move.result.delta_cost);
            exit(14);
        }
#endif
        cost = room_capacity_cost + min_working_days_cost +
                curriculum_compactness_cost + room_stability_cost;

//#if DEBUG
//        if (solution_cost(&s) != cost) {
//            eprint("solution_cost(&s) != cost");
//            exit(1);
//        }
//#endif

        debug("Moved to solution (rc=%d, mwd=%d, cc=%d, rs=%d) of cost = %d (best is %d)",
              room_capacity_cost, min_working_days_cost,
              curriculum_compactness_cost, room_stability_cost, cost, best_sol_cost);


        if (cost == last_cost) {
            swap_move_pick_randomizer = swap_move_pick_randomizer == 0 ? 0.5 : swap_move_pick_randomizer * 2;
            debug("Detected same cost, swap_move_pick_randomizer = %g", swap_move_pick_randomizer);
        }
        else {
            if (swap_move_pick_randomizer > 0)
                swap_move_pick_randomizer *= 0.5;
            else
                swap_move_pick_randomizer = 0;
        }

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

    return strempty(solver->error);
}
