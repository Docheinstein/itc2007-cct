#include "hybrid_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/assert_utils.h>
#include <utils/common_utils.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

static bool do_hybrid_neighbourhood_swap(solution *sol, int *cost, int iter) {
    int prev_cost = *cost;
    bool improved;
    int i = 0;

    do {
        int j = 0;

        improved = false;

        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, sol);

        neighbourhood_swap_move swap_mv;
        neighbourhood_swap_result swap_result;

        neighbourhood_swap_move best_swap_mv;
        int best_swap_cost = INT_MAX;

        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            debug2("[%d.swap.%d.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
                   iter, i, j,
                   swap_mv.c1, swap_mv.r1, swap_mv.d1,
                   swap_mv.s1, swap_mv.r2, swap_mv.d2, swap_mv.s2);

            assert(swap_mv.c1 >= swap_mv.c2);

            neighbourhood_swap(sol, &swap_mv,
                                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                                   NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                                   NEIGHBOURHOOD_PREDICT_NEVER,
                                   NEIGHBOURHOOD_PERFORM_NEVER,
                                   &swap_result);
            if (swap_result.feasible &&
//                swap_result.delta_cost < best_swap_cost) {
                swap_result.delta_cost < best_swap_cost) {
                debug2("[%d.swap.%d.%d] New best candidate swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> %d",
                       iter, i, j, swap_mv.c1, swap_mv.r1, swap_mv.d1,
                       swap_mv.s1, swap_mv.r2, swap_mv.d2, swap_mv.s2, swap_result.delta_cost);
                best_swap_cost = swap_result.delta_cost;
                best_swap_mv = swap_mv;
            }
            j++;
        }

        if (best_swap_cost < 0) {
            debug("[%d.swap.%d] best swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> %d",
                   iter, i, best_swap_mv.c1, best_swap_mv.r1, best_swap_mv.d1,
                   best_swap_mv.s1, best_swap_mv.r2, best_swap_mv.d2, best_swap_mv.s2, best_swap_cost);

            neighbourhood_swap(sol, &best_swap_mv,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);
            debug("[%d.swap.%d] LS: solution improved (by %d (rc=%d mwd=%d, cc=%d, rs=%d))",
                  iter, i, swap_result.delta_cost,
                  swap_result.delta_cost_room_capacity,
                  swap_result.delta_cost_min_working_days,
                  swap_result.delta_cost_curriculum_compactness,
                  swap_result.delta_cost_room_stability);
            improved = true;
            *cost += swap_result.delta_cost;
        }

        neighbourhood_swap_iter_destroy(&swap_iter);
        i++;
    } while (cost > 0 && improved);

    return *cost < prev_cost;
}

static void do_hybrid_complete(solution *sol) {
    int cost = solution_cost(sol);
    int iter = 0;
    bool improved;
    do {
        do {
            debug("[%d] Starting cost: %d", iter, cost);
            assert(solution_cost(sol) == cost);
            assert(solution_satisfy_hard_constraints(sol));

//            render_solution(sol, "/tmp/sol-before.png");
//            write_solution(sol, "/tmp/sol-before");

            bool i1 = do_hybrid_neighbourhood_swap(sol, &cost, iter);

//            render_solution(sol, "/tmp/sol-after.png");
//            write_solution(sol, "/tmp/sol-after");

            assert(solution_cost(sol) == cost);
            assert(solution_satisfy_hard_constraints(sol));

            debug("[%d] After N1 cost: %d", iter, cost);
#if 0
            bool i2 = do_hybrid_neighbourhood_stabilize_room(sol, &cost, iter);
            assert(solution_cost(sol) == cost);
            debug("[%d] After N1,N2 cost: %d", iter, cost);
#else
            bool i2 = false;
#endif

            improved = i1 || i2;
            iter++;
        } while (cost > 0 && improved);
        assert(solution_satisfy_hard_constraints(sol));

        if (solution_cost(sol) != cost) {

            render_solution(sol, "/tmp/sol-wrong.png");
            write_solution(sol, "/tmp/sol-wrong");
            fail("xExpected %d, found %d", cost, solution_cost(sol));
        }
#if 0
        debug("[%d] Trying with depth2-fast before giving up", iter);
        assert(solution_satisfy_hard_constraints(sol));

        if (do_hybrid_neighbourhood_swap_depth2(sol, &cost, iter, true)) {

            if (solution_cost(sol) != cost) {
                render_solution(sol, "/tmp/sol-wrong.png");
                write_solution(sol, "/tmp/sol-wrong");
                fail("yExpected %d, found %d", cost, solution_cost(sol));
            }
            continue;
        }
        assert(solution_satisfy_hard_constraints(sol));

        debug("[%d] After N1^2 fast cost: %d", iter, cost);

        debug("rc=%d", solution_room_capacity_cost(sol));
        debug("mwd=%d", solution_min_working_days_cost(sol));
        debug("c=%d", solution_curriculum_compactness_cost(sol));
        debug("rs=%d", solution_room_stability_cost(sol));
        write_solution(sol, "/tmp/sol-right");
        render_solution(sol, "/tmp/sol-right.png");

        debug("[%d] Trying with depth2-accurate before giving up", iter);
        if (!do_hybrid_neighbourhood_swap_depth2(sol, &cost, iter, false))
            break;

        debug("[%d] After N1^2 cost: %d", iter, cost);
        assert(solution_satisfy_hard_constraints(sol));

        if (solution_cost(sol) != cost) {
            write_solution(sol, "/tmp/sol-wrong");
            render_solution(sol, "/tmp/sol-wrong.png");
            debug("rc=%d", solution_room_capacity_cost(sol));
            debug("mwd=%d", solution_min_working_days_cost(sol));
            debug("c=%d", solution_curriculum_compactness_cost(sol));
            debug("rs=%d", solution_room_stability_cost(sol));
            fail("Expected %d, found %d", cost, solution_cost(sol));
        }
#else
        if (!improved)
            break;
#endif
        assert(solution_cost(sol) == cost);
        iter++;
    } while (true);
}

static void do_hybrid_fast_descend(solution *sol) {
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

                debug2("[%d.swap.%d] LS swap(c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d))",
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
                    debug2("[%d.swap.%d] LS: solution improved (by %d), cost = %d",
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

void hybrid_solver_config_init(hybrid_solver_config *config) {
    config->multistart = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void hybrid_solver_config_destroy(hybrid_solver_config *config) {

}

void hybrid_solver_init(hybrid_solver *solver) {
    solver->error = NULL;
}

void hybrid_solver_destroy(hybrid_solver *solver) {
    free(solver->error);
}

const char *hybrid_solver_get_error(hybrid_solver *solver) {
    return solver->error;
}

void hybrid_solver_reinit(hybrid_solver *solver) {
    hybrid_solver_destroy(solver);
    hybrid_solver_init(solver);
}

bool hybrid_solver_solve(hybrid_solver *solver,
                               hybrid_solver_config *config,
                               solution *sol_out) {
    const model *model = sol_out->model;
    hybrid_solver_reinit(solver);

    long starting_time = ms();
    long time_limit = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int multistart = config->multistart;

    if (!(time_limit > 0) && !(multistart > 0)) {
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
        if (time_limit > 0 && !(ms() < time_limit)) {
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
            feasible_solution_finder finder;
            feasible_solution_finder_init(&finder);

            feasible_solution_finder_config finder_config;
            feasible_solution_finder_config_init(&finder_config);
            finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

            feasible_solution_finder_find(&finder, &finder_config, &s);
        }

        debug("[%d] Initial solution found, cost = %d", iter, solution_cost(&s));

        // Local search starting from this feasible solution
        do_hybrid_complete(&s);

        // Check if the solution found is better than the best known
        // TODO: can be avoid to recompute cost
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
