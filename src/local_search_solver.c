#include "local_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include "neighbourhood.h"
#include "feasible_solution_finder.h"

static void do_local_search(const solution *sol_in, solution *sol_out) {
    solution_copy(sol_out, sol_in);
    int cost = solution_cost(sol_out);

    int i = 0;
    bool improved;
    do {
        improved = false;

        neighbourhood_iter iter;
        neighbourhood_iter_init(&iter, sol_out);

        int c1, r1, d1, s1, r2, d2, s2;

        int j = 0;
        while (neighbourhood_iter_next(&iter, &c1, &r1, &d1, &s1, &r2, &d2, &s2)) {
            bool restart = false;

            solution sol_ls;
            solution_init(&sol_ls, sol_in->model);
            debug2(" [%d.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d) )", i, j, c1, r1, d1, s1, r2, d2, s2);

            neighbourhood_swap_assignment(sol_out, &sol_ls, c1, r1, d1, s1, r2, d2, s2);
            int ls_cost = solution_cost(&sol_ls);

            if (solution_satisfy_hard_constraints(&sol_ls) && ls_cost < cost) {
                solution_copy(sol_out, &sol_ls);
                cost = ls_cost;
                debug("  [%d.%d] LS: solution improved, cost = %d", i, j, cost);
                improved = true;
                restart = true;
            }

            solution_destroy(&sol_ls);
            j++;

            if (restart)
                break;
        }

        neighbourhood_iter_destroy(&iter);
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
    local_search_solver_reinit(solver);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);
    finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

    int iters = config->multistart > 1 ? config->multistart : 1;

    int best_solution_cost = INT_MAX;

    for (int i = 0; i < iters && best_solution_cost > 0; i++) {
        solution s;
        solution_init(&s, sol_out->model);

        debug("[%d] Finding initial feasible solution...", i);
        if (feasible_solution_finder_find(&finder, &finder_config, &s)) {
            int cost = solution_cost(&s);

            debug("[%d] Initial solution is feasible, cost = %d", i, cost);

            // Local search starting from this feasible solution
            solution s_ls;
            solution_init(&s_ls, sol_out->model);

            do_local_search(&s, &s_ls);

            // Check if the solution found is better than the best known
            int ls_cost = solution_cost(&s_ls);
            if (ls_cost < best_solution_cost) {
                verbose("[%d] LS: new best solution found, cost = %d", i, ls_cost);
                best_solution_cost = ls_cost;
                solution_copy(sol_out, &s_ls);
            }

            solution_destroy(&s_ls);
        }

        solution_destroy(&s);
    }

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
