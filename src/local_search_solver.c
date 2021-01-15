#include "local_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include "neighbourhood.h"
#include "feasible_solution_finder.h"

static void do_local_search_complete(solution *sol) {
    neighbourhood_swap_result swap_result;

    int cost = solution_cost(sol);

    int i = 0;
    bool improved;
    do {
        int c1, r1, d1, s1, r2, d2, s2;
        int j = 0;
        improved = false;

        solution sol_neigh;
        solution_init(&sol_neigh, sol->model);
        solution_copy(&sol_neigh, sol);

        neighbourhood_swap_iter iter;
        neighbourhood_swap_iter_init(&iter, &sol_neigh);

        int best_c1, best_r1, best_d1, best_s1, best_r2, best_d2, best_s2;
        int best_delta = INT_MAX;

        while (neighbourhood_swap_iter_next(&iter, &c1, &r1, &d1, &s1, &r2, &d2, &s2)) {
            debug("[%d.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   i, j, c1, r1, d1, s1, r2, d2, s2);

            neighbourhood_swap(&sol_neigh, c1, r1, d1, s1, r2, d2, s2,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.delta_cost < best_delta) {
                debug("[%d.%d] LS: new candidate for best neighbourhood move, delta = %d", i, j, swap_result.delta_cost);
                best_c1 = c1;
                best_r1 = r1;
                best_d1 = d1;
                best_s1 = s1;
                best_r2 = r2;
                best_d2 = d2;
                best_s2 = s2;
                best_delta = swap_result.delta_cost;
            }

            j++;
        }

        if (best_delta < 0) {
            neighbourhood_swap(&sol_neigh, best_c1, best_r1, best_d1, best_s1, best_r2, best_d2, best_s2,
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
    neighbourhood_swap_result swap_result;

    int cost = solution_cost(sol);

    int i = 0;
    bool improved;
    do {
        int c1, r1, d1, s1, r2, d2, s2;
        int j = 0;
        improved = false;

        solution sol_neigh;
        solution_init(&sol_neigh, sol->model);
        solution_copy(&sol_neigh, sol);

        neighbourhood_swap_iter iter;
        neighbourhood_swap_iter_init(&iter, &sol_neigh);

        while (neighbourhood_swap_iter_next(&iter, &c1, &r1, &d1, &s1, &r2, &d2, &s2)) {
            bool restart = false;

            debug("[%d.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   i, j, c1, r1, d1, s1, r2, d2, s2);

            if (neighbourhood_swap(&sol_neigh, c1, r1, d1, s1, r2, d2, s2,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
                               &swap_result)) {
                // Feasible solution after swap
                int neigh_cost = cost + swap_result.delta_cost;
                debug("[%d.%d] LS: solution improved, cost = %d", i, j, neigh_cost);

                cost = neigh_cost;
                solution_copy(sol, &sol_neigh);
                improved = true;
                restart = true;
            }

            j++;

            if (restart)
                break;
        }

        solution_destroy(&sol_neigh);
        neighbourhood_swap_iter_destroy(&iter);
        i++;
    } while (cost > 0 && improved);
}

void local_search_solver_config_init(local_search_solver_config *config) {
    config->multistart = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
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
    debug("Starting time = %lu", starting_time);
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int multistart = config->multistart;
        debug("deadline = %lu", deadline);

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

        debug("[%d] Finding initial feasible solution...", iter);
        if (feasible_solution_finder_find(&finder, &finder_config, &s)) {
            debug("[%d] Initial solution is feasible, cost = %d", iter, solution_cost(&s));

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
        }

        solution_destroy(&s);
    }

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
