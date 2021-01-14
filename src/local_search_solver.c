#include "local_search_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/io_utils.h>
#include "neighbourhood.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

static void do_local_search(solution *sol) {
#if DEBUG
    if (solution_fingerprint(sol) != solution_fingerprint(sol)) {
        eprint("Different fingerprint after solution copy");
        exit(4);
    }
#endif

    neighbourhood_swap_result swap_result;

    int cost = solution_cost(sol);

    int i = 0;
    bool improved;
    do {
        debug("Begin LS major cycle %d", i);

        int c1, r1, d1, s1, c2, r2, d2, s2;
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

//            render_solution_overview(&sol_neigh, "/tmp/before-swap.png");
#if 0
            debug("---- BEFORE SWAP ----");
            print_solution(&sol_neigh);
            char tmp[256];
            snprintf(tmp, 256, "/tmp/%d-%d_a_before-swap.png", i, j);
            render_solution_overview(&sol_neigh, tmp);
#endif

#if DEBUG
            int cost1 = solution_room_capacity_cost(&sol_neigh);
            int cost2 = solution_min_working_days_cost(&sol_neigh);
            int cost3 = solution_curriculum_compactness_cost(&sol_neigh);
            int cost4 = solution_room_stability_cost(&sol_neigh);
            debug("cost1=%d", cost1);
            debug("cost2=%d", cost2);
            debug("cost3=%d", cost3);
            debug("cost4=%d", cost4);
#endif

//          render_solution_overview(&sol_neigh, "/tmp/sol-before.png");
            neighbourhood_swap(&sol_neigh, c1, r1, d1, s1, r2, d2, s2, &swap_result);
            if (swap_result.feasible) {
                // Feasible solution after swap
                debug("[%d.%d] Performed feasible LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)",
                      i, j, c1, r1, d1, s1, r2, d2, s2);

#if DEBUG
                if (!solution_satisfy_hard_constraints(&sol_neigh)) {
                    eprint("After valid swap, solution must satisfy hard constraints");
                    render_solution_overview(&sol_neigh, "/tmp/failure.png");
                    exit(3);
                }

                int cost1_after = solution_room_capacity_cost(&sol_neigh);
                int cost2_after = solution_min_working_days_cost(&sol_neigh);
                int cost3_after = solution_curriculum_compactness_cost(&sol_neigh);
                int cost4_after = solution_room_stability_cost(&sol_neigh);
                debug("cost1_after=%d", cost1);
                debug("cost2_after=%d", cost2);
                debug("cost3_after=%d", cost3);
                debug("cost4_after=%d", cost4);


                if (cost1_after != cost1 + swap_result.delta_cost_room_capacity) {
                    eprint("Computed wrong RoomCapacity cost; expected %d - found %d + %d",
                           cost1_after, cost1, swap_result.delta_cost_room_capacity);
                    exit(1);
                }

                if (cost2_after != cost2 + swap_result.delta_cost_min_working_days) {
                    eprint("Computed wrong MinWorkingDays cost; expected %d - found %d + %d",
                           cost2_after, cost2, swap_result.delta_cost_min_working_days);
                    render_solution_overview(&sol_neigh, "/tmp/sol.png");
                    write_solution(&sol_neigh, "/tmp/sol.ctt.sol");
                    exit(1);
                }

                if (cost3_after != cost3 + swap_result.delta_cost_curriculum_compactness) {
                    eprint("Computed wrong CurCompactness cost; expected %d - found %d + %d",
                           cost3_after, cost3, swap_result.delta_cost_curriculum_compactness);
                    render_solution_overview(&sol_neigh, "/tmp/sol.png");
                    exit(1);
                }

                if (cost4_after != cost4 + swap_result.delta_cost_room_stability) {
                    eprint("Computed wrong RoomStability cost; expected %d - found %d + %d",
                           cost4_after, cost4, swap_result.delta_cost_room_stability);
                    render_solution_overview(&sol_neigh, "/tmp/sol.png");
                    exit(1);
                }
#endif
                int neigh_cost = cost + swap_result.delta_cost;

#if DEBUG
                int neigh_cost_real = solution_cost(&sol_neigh);
                if (neigh_cost_real != neigh_cost) {
                    debug("Expected cost = %d, found = %d", neigh_cost_real, neigh_cost);
                    exit(1);
                }
#endif

                if (neigh_cost < cost) {
                    debug("[%d.%d] LS: solution improved, cost = %d", i, j, neigh_cost);
                    cost = neigh_cost;
                    solution_copy(sol, &sol_neigh);
                    improved = true;
                    restart = true;
                } else {
                    debug("[%d.%d] LS: solution NOT improved, cost %d >= %d", i, j, neigh_cost, cost);

                    // Swap back
                    debug("[%d.%d] swap back", i, j);
                    neighbourhood_swap_back(&sol_neigh, c1, r1, d1, s1, swap_result.c2, r2, d2, s2);
                }
            } else {
#if 0
                debug("[%d.%d] Skipped infeasible LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)",
                      i, j, c1, r1, d1, s1, r2, d2, s2);

                if (solution_satisfy_hard_constraints(&sol_neigh)) {
                    eprint("After invalid swap, solution must NOT satisfy hard constraints");
                    render_solution_overview(sol, "/tmp/sol_before_neigh.png");
                    render_solution_overview(&sol_neigh, "/tmp/sol_after_neigh.png");
                    exit(3);
                }
#endif
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

    int iters = config->multistart > 1 ? config->multistart : 1;

    int best_solution_cost = INT_MAX;

    int iter = 0;
    while (iter < iters && best_solution_cost > 0) {
        solution s;
        solution_init(&s, model);

        debug("[%d] Finding initial feasible solution...", iter);
        if (feasible_solution_finder_find(&finder, &finder_config, &s)) {
            debug("[%d] Initial solution is feasible, cost = %d", iter, solution_cost(&s));

            // Local search starting from this feasible solution
            do_local_search(&s);

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
