#include "local_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <neighbourhoods/neighbourhood_stabilize_room.h>
#include "neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"

static void do_local_search_complete(solution *sol) {
    neighbourhood_swap_result swap_result;

    int cost = solution_cost(sol);

    int i = 0;
    bool improved;
    do {
        int j = 0;
        improved = false;

        solution sol_neigh;
        solution_init(&sol_neigh, sol->model);
        solution_copy(&sol_neigh, sol);

        neighbourhood_swap_iter iter;
        neighbourhood_swap_iter_init(&iter, &sol_neigh);

        neighbourhood_swap_move mv;

        neighbourhood_swap_move best_mv;
        int best_delta = INT_MAX;

        while (neighbourhood_swap_iter_next(&iter, &mv)) {
            debug("[%d.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   i, j, mv.c1, mv.r1, mv.d1, mv.s1, mv.r2, mv.d2, mv.s2);

            neighbourhood_swap(&sol_neigh, &mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.delta_cost < best_delta) {
                debug("[%d.%d] LS: new candidate for best neighbourhood move, delta = %d", i, j, swap_result.delta_cost);
                best_mv = mv;
                best_delta = swap_result.delta_cost;
            }

            j++;
        }

        if (best_delta < 0) {
            neighbourhood_swap(&sol_neigh, &best_mv,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);

            int neigh_cost = cost + swap_result.delta_cost;
            cost = neigh_cost;
            solution_copy(sol, &sol_neigh);
            improved = true;
            debug("[%d.%d] LS: solution improved, cost = %d", i, j, cost);
        }

        solution_destroy(&sol_neigh);
        neighbourhood_swap_iter_destroy(&iter);
        i++;
    } while (cost > 0 && improved);
}

static void do_local_search_fast_descend(solution *sol) {
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

                debug("[%d.swap.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
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
                    debug("[%d.swap.%d] LS: solution improved (by %d), cost = %d",
                          i, jj, swap_result.delta_cost, neigh_cost);

                    cost = neigh_cost;
                    improved = true;
                    break;
                }
            }

            neighbourhood_swap_iter_destroy(&swap_iter);
        } while (cost > 0 && improved);

        // N2: stabilize_room, do only one improvement

        int j = 0;

        neighbourhood_stabilize_room_iter stabilize_room_iter;
        neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, sol);

        neighbourhood_stabilize_room_move stabilize_room_mv;
        neighbourhood_stabilize_room_result stabilize_room_result;

        while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter, &stabilize_room_mv)) {
            int jj = j++;

            debug("[%d.stabilize_room.%d] LS stabilize_room(c=%d r=%d)",
                   i, jj, stabilize_room_mv.c1, stabilize_room_mv.r2);

            if (neighbourhood_stabilize_room(sol, &stabilize_room_mv,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
                               &stabilize_room_result)) {

                stabilize_room_improvements++;
                stabilize_room_improvements_delta += stabilize_room_result.delta_cost;
                int neigh_cost = cost + stabilize_room_result.delta_cost;
                debug("[%d.stabilize_room.%d] LS: solution improved (by %d), cost = %d",
                      i, jj, stabilize_room_result.delta_cost, neigh_cost);

                cost = neigh_cost;
                improved = true;
#if DEBUG
                if (solution_cost(sol) != cost) {
                    exit(21);
                }
#endif
                break;
            }
        }

        neighbourhood_stabilize_room_iter_destroy(&stabilize_room_iter);

        i++;
    } while (cost > 0 && improved);

    debug("swap_improvements = %d", swap_improvements);
    debug("stabilize_room_improvements = %d", stabilize_room_improvements);
    debug("stabilize_room_improvements_delta = %d", stabilize_room_improvements_delta);
}

void local_search_solver_config_init(local_search_solver_config *config) {
    config->multistart = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void local_search_solver_config_destroy(local_search_solver_config *config) {

}

void local_search_solver_init(local_search_solver *solver) {
    solver->error = NULL;
}

void local_search_solver_destroy(local_search_solver *solver) {
    free(solver->error);
}

const char *local_search_solver_get_error(local_search_solver *solver) {
    return solver->error;
}

void local_search_solver_reinit(local_search_solver *solver) {
    local_search_solver_destroy(solver);
    local_search_solver_init(solver);
}

bool local_search_solver_solve(local_search_solver *solver,
                               local_search_solver_config *config,
                               solution *sol_out) {
    const model *model = sol_out->model;
    local_search_solver_reinit(solver);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);
    finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

    long starting_time = ms();
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int multistart = config->multistart;

    if (!(deadline > 0) && !(multistart > 0)) {
        print("WARN: you should probably use either "
              "time limit (-t) or multistart (-n) for heuristics methods.\n"
              "Will do a single run...");
        multistart = 1;
    }

    int best_solution_cost = INT_MAX;

    int iter = 0;
    while (best_solution_cost > 0) {
        if (multistart > 0 && !(iter < multistart)) {
            debug("Multistart limit reached (%d), stopping here", iter);
            break;
        }
        if (deadline > 0 && !(ms() < deadline)) {
            debug("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }

        solution s;
        solution_init(&s, model);

        if (config->starting_solution) {
            debug("[%d] Using provided initial solution", iter);
            solution_copy(&s, config->starting_solution);
        }
        else {
            debug("[%d] Finding initial feasible solution...", iter);
            feasible_solution_finder_find(&finder, &finder_config, &s);
        }

        debug("[%d] Initial solution found, cost = %d", iter, solution_cost(&s));

        // Local search starting from this feasible solution
        do_local_search_fast_descend(&s);

        // Check if the solution found is better than the best known
        int cost = solution_cost(&s);
        if (cost < best_solution_cost) {
            verbose("[%d] LS: new best solution found, cost = %d", iter, cost);
            best_solution_cost = cost;
            solution_copy(sol_out, &s);
        }

        iter++;

        solution_destroy(&s);
    }

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
