#include "tabu_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include "neighbourhood.h"
#include "feasible_solution_finder.h"

void tabu_search_solver_config_init(tabu_search_solver_config *config) {
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
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

bool tabu_search_solver_solve(tabu_search_solver *solver,
                              tabu_search_solver_config *config,
                              solution *sol_out) {
    const model *model = sol_out->model;
    tabu_search_solver_reinit(solver);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);
    finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

    long starting_time = ms();
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;

//    if (deadline > 0 && !(ms() < deadline)) {
//        debug("Time limit reached (%ds), stopping here", config->time_limit);
//        break;
//    }

    int best_solution_cost = INT_MAX;

    solution s;
    solution_init(&s, model);

    debug("Finding initial feasible solution...");
    feasible_solution_finder_find(&finder, &finder_config, &s);
    debug("Initial solution is feasible, cost = %d", solution_cost(&s));

    // Tabu search starting from this feasible solution
#if 0
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

        neighbourhood_swap_move mv;

        int best_delta = INT_MAX;

        while (neighbourhood_swap_iter_next(&iter, &mv)) {
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
#endif

    solution_destroy(&s);

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
