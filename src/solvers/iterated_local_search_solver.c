#include "iterated_local_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/assert_utils.h>
#include <utils/mem_utils.h>
#include <utils/random_utils.h>
#include <utils/array_utils.h>
#include "neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

static bool do_iterated_local_search_neighbourhood_swap(
        solution *sol, int *cost, int iter,
        int (*swap_result_comparator)(const neighbourhood_swap_result *, const neighbourhood_swap_result *)) {

    CRDSQT(sol->model);

    int prev_cost = *cost;

    neighbourhood_swap_move *best_swap_moves =
                mallocx(solution_assignment_count(sol) * R * D * S,
                        sizeof(neighbourhood_swap_move));

    int best_swap_moves_cursor = 0;
    neighbourhood_swap_result best_swap_result;

    int j = 0;

    neighbourhood_swap_iter swap_iter;
    neighbourhood_swap_iter_init(&swap_iter, sol);

    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result = {
            .delta_cost = INT_MAX,
            .delta_cost_room_capacity = INT_MAX,
            .delta_cost_min_working_days = INT_MAX,
            .delta_cost_curriculum_compactness = INT_MAX,
            .delta_cost_room_stability = INT_MAX
    };

    while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
        debug2("[%d.swap.%d] ILS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
               iter, j,
               swap_mv.c1, swap_mv.r1, swap_mv.d1,
               swap_mv.s1, swap_mv.r2, swap_mv.d2, swap_mv.s2);

        neighbourhood_swap(sol, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);
        if (swap_result.feasible) {
            if (swap_result_comparator(&swap_result, &best_swap_result) <= 0) {
                if (swap_result_comparator(&swap_result, &best_swap_result) < 0) {
                    best_swap_moves_cursor = 0;
                    best_swap_result = swap_result;
                }
                best_swap_moves[best_swap_moves_cursor++] = swap_mv;
            }
        }
        j++;
    }

    if (best_swap_moves_cursor == 0)
        return false;

    debug("[%d] ILS: Picking from %d possible moves, best of its kind is %d (rc=%d, mwd=%d, cc=%d, rs=%d)",
          iter, best_swap_moves_cursor,
          best_swap_result.delta_cost,
          best_swap_result.delta_cost_room_capacity, best_swap_result.delta_cost_min_working_days,
          best_swap_result.delta_cost_curriculum_compactness, best_swap_result.delta_cost_room_stability);

    int pick = rand_int_range(0, best_swap_moves_cursor);
    neighbourhood_swap_move m = best_swap_moves[pick];

    neighbourhood_swap(sol, &m,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PERFORM_ALWAYS,
                       &swap_result);
    *cost += swap_result.delta_cost;

    neighbourhood_swap_iter_destroy(&swap_iter);

    free(best_swap_moves);

    return *cost < prev_cost;
}


static bool do_iterated_local_search_neighbourhood_stabilize_room(solution *sol, int *cost, int iter) {
    int prev_cost = *cost;
    bool improved;
    int i = 0;

    do {
        int j = 0;

        improved = false;

        neighbourhood_stabilize_room_iter stabilize_room_iter;
        neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, sol);

        neighbourhood_stabilize_room_move stabilize_room_mv;
        neighbourhood_stabilize_room_result stabilize_room_result;

        neighbourhood_stabilize_room_move best_stabilize_room_mv;
        int best_stabilize_room_cost = INT_MAX;

        while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter, &stabilize_room_mv)) {
            debug2("[%d.stabilize_room.%d.%d] ILS stabilize_room(c=%d r=%d)",
                   iter, i, j,
                   stabilize_room_mv.c1, stabilize_room_mv.r2);

            neighbourhood_stabilize_room(sol, &stabilize_room_mv,
                                   NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                                   NEIGHBOURHOOD_PREDICT_NEVER,
                                   NEIGHBOURHOOD_PERFORM_NEVER,
                                   &stabilize_room_result);
            if (stabilize_room_result.delta_cost < best_stabilize_room_cost) {
                debug2("[%d.stabilize_room.%d.%d] New best candidate stabilize_room(c=%d r=%d) -> %d",
                       iter, i, j, stabilize_room_mv.c1, stabilize_room_mv.r2, stabilize_room_result.delta_cost);
                best_stabilize_room_cost = stabilize_room_result.delta_cost;
                best_stabilize_room_mv = stabilize_room_mv;
            }
            j++;
        }

        if (best_stabilize_room_cost < 0) {
            neighbourhood_stabilize_room(sol, &best_stabilize_room_mv,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &stabilize_room_result);
            debug("[%d.stabilize_room.%d] ILS: solution improved (by %d)",
                  iter, i, best_stabilize_room_cost);
            improved = true;
            *cost += best_stabilize_room_cost;
        }

        neighbourhood_stabilize_room_iter_destroy(&stabilize_room_iter);
        i++;
    } while (cost > 0 && improved);

    return *cost < prev_cost;
}

static void do_iterated_local_search_fast_descend(solution *sol) {
    int cost = solution_cost(sol);

    int swap_improvements = 0;
    int stabilize_room_improvements = 0;
    int stabilize_room_improvements_delta = 0;

    int i = 0;
    bool improved;
    do {
        // N1: swap: improve as much as possible
        do {
            int j = 0;
            improved = false;

            neighbourhood_swap_iter swap_iter;
            neighbourhood_swap_iter_init(&swap_iter, sol);

            neighbourhood_swap_move swap_mv;
            neighbourhood_swap_result swap_result;

            while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
                int jj = j++;

                debug2("[%d.swap.%d] ILS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                      i, jj,
                      swap_mv.c1, swap_mv.r1, swap_mv.d1,
                      swap_mv.s1, swap_mv.r2, swap_mv.d2, swap_mv.s2);

                if (neighbourhood_swap(sol, &swap_mv,
                                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                                       NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
                                       &swap_result)) {
                    swap_improvements++;
                    int neigh_cost = cost + swap_result.delta_cost;
                    debug2("[%d.swap.%d] ILS: solution improved (by %d), cost = %d",
                          i, jj, swap_result.delta_cost, neigh_cost);

                    cost = neigh_cost;
                    improved = true;
                    break;
                }
            }

            neighbourhood_swap_iter_destroy(&swap_iter);
        } while (cost > 0 && improved);

        // N2: stabilize_room, do only one improvement
#if 0
        int j = 0;

        neighbourhood_stabilize_room_iter stabilize_room_iter;
        neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, sol);

        neighbourhood_stabilize_room_move stabilize_room_mv;
        neighbourhood_stabilize_room_result stabilize_room_result;

        while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter, &stabilize_room_mv)) {
            int jj = j++;

            debug("[%d.stabilize_room.%d] ILS stabilize_room(c=%d r=%d)",
                   i, jj, stabilize_room_mv.c1, stabilize_room_mv.r2);

            if (neighbourhood_stabilize_room(sol, &stabilize_room_mv,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
                               &stabilize_room_result)) {

                stabilize_room_improvements++;
                stabilize_room_improvements_delta += stabilize_room_result.delta_cost;
                int neigh_cost = cost + stabilize_room_result.delta_cost;
                debug("[%d.stabilize_room.%d] ILS: solution improved (by %d), cost = %d",
                      i, jj, stabilize_room_result.delta_cost, neigh_cost);

                cost = neigh_cost;
                improved = true;

                assert(solution_cost(sol) == cost);
                break;
            }
        }

        neighbourhood_stabilize_room_iter_destroy(&stabilize_room_iter);
#endif
        i++;
    } while (cost > 0 && improved);

    debug("swap_improvements = %d", swap_improvements);
    debug("stabilize_room_improvements = %d", stabilize_room_improvements);
    debug("stabilize_room_improvements_delta = %d", stabilize_room_improvements_delta);
}

void iterated_local_search_solver_config_init(iterated_local_search_solver_config *config) {
    config->idle_limit = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void iterated_local_search_solver_config_destroy(iterated_local_search_solver_config *config) {

}

void iterated_local_search_solver_init(iterated_local_search_solver *solver) {
    solver->error = NULL;
}

void iterated_local_search_solver_destroy(iterated_local_search_solver *solver) {
    free(solver->error);
}

const char *iterated_local_search_solver_get_error(iterated_local_search_solver *solver) {
    return solver->error;
}

void iterated_local_search_solver_reinit(iterated_local_search_solver *solver) {
    iterated_local_search_solver_destroy(solver);
    iterated_local_search_solver_init(solver);
}

bool iterated_local_search_solver_solve(iterated_local_search_solver *solver,
                               iterated_local_search_solver_config *config,
                               solution *s) {
    const model *model = s->model;
    iterated_local_search_solver_reinit(solver);

    long starting_time = ms();
    long time_limit = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int idle_limit = config->idle_limit;

    if (!(time_limit > 0) && !(idle_limit > 0)) {
        print("WARN: you should probably use either "
              "time limit (-t) or idle_limit (-n) for hill climbing method.\n"
              "Will do a single run...");
        idle_limit = 1;
    }

    int best_solution_cost = INT_MAX;

    int idle_iter = 0;
    int iter = 0;

    int (*comparators[])(const neighbourhood_swap_result *, const neighbourhood_swap_result *) = {
//        neighbourhood_swap_move_compare_cost,
        neighbourhood_swap_move_compare_room_capacity_cost,
        neighbourhood_swap_move_compare_room_min_working_days_cost,
        neighbourhood_swap_move_compare_room_stability_cost,
        neighbourhood_swap_move_compare_room_curriculum_compactness_cost,
        neighbourhood_swap_move_compare_room_curriculum_compactness_cost,
        neighbourhood_swap_move_compare_room_curriculum_compactness_cost,
    };

    if (config->starting_solution) {
        debug("[%d] Using provided initial solution", iter);
        solution_copy(s, config->starting_solution);
    }
    else {
        debug("[%d] Finding initial feasible solution...", iter);
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

        feasible_solution_finder_find(&finder, &finder_config, s);
    }

    int cost = solution_cost(s);

    while (best_solution_cost > 0) {
        if (idle_limit > 0 && !(idle_iter < idle_limit)) {
            verbose("Idle limit reached (%d) at iter %d, stopping here", idle_iter, iter);
            break;
        }
        if (time_limit > 0 && !(ms() < time_limit)) {
            verbose("Time limit reached (%ds) at iter %d, stopping here", config->time_limit, iter);
            break;
        }

//        assert(cost == solution_cost(s));
        verbose("[%d] %d   // best is %d", iter, cost, best_solution_cost);

        // Local search starting from this feasible solution
        debug("[%d] Before local search cost = %d", iter, cost);
        while (do_iterated_local_search_neighbourhood_swap(s, &cost, iter, neighbourhood_swap_move_compare_cost));
        debug("[%d] After local search, cost = %d", iter, cost);

        int r = rand_int_range(0, LENGTH(comparators));
        do_iterated_local_search_neighbourhood_swap(s, &cost, iter, comparators[r]);
        debug("[%d] After comparator[%d], cost = %d", iter, r, cost);
        assert(cost == solution_cost(s));

        // Check if the solution found is better than the best known
//        cost = solution_cost(s);
        if (cost < best_solution_cost) {
            verbose("[%d] ILS: new best solution found, cost = %d", iter, cost);
            best_solution_cost = cost;
            idle_iter = 0;
        } else {
            idle_iter++;
        }

        iter++;
    }

//    solution_destroy(s);

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
