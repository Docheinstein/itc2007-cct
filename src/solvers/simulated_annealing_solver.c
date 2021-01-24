#include "simulated_annealing_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <heuristics/neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/assert_utils.h>
#include <utils/mem_utils.h>
#include <utils/random_utils.h>
#include <utils/array_utils.h>
#include <math.h>
#include <utils/math_utils.h>
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

static void do_simulated_annealing_phase(
        solution *sol, int *cost, int best_cost, int iter, int temperature, neighbourhood_swap_move *swap_moves) {

    CRDSQT(sol->model);

    int swap_moves_cursor = 0;

    neighbourhood_swap_iter swap_iter;
    neighbourhood_swap_iter_init(&swap_iter, sol);

    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result;

    while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
        neighbourhood_swap(sol, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);
        if (swap_result.feasible) {
            swap_moves[swap_moves_cursor++] = swap_mv;
        }
    }

    int pick = rand_int_range(0, swap_moves_cursor);
    neighbourhood_swap_move m = swap_moves[pick];


    neighbourhood_swap(sol, &m,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PERFORM_NEVER,
                       &swap_result);
    int next_cost = *cost + swap_result.delta_cost;

    debug("[%d] SA pick: (c=%d (r=%d d=%d s=%d) (r=%d d=%d s=%d)) -> %d (delta %d)",
       iter,
       m.c1, m.r1, m.d1,
       m.s1, m.r2, m.d2, m.s2, next_cost, swap_result.delta_cost);

    if (next_cost < best_cost) {
        debug("[%d] SA found move that lead to best solution of cost %d, doing it", iter, next_cost);
        neighbourhood_swap(sol, &m,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PERFORM_ALWAYS,
                           &swap_result);
        *cost = next_cost;
    }
    else {
        static const double BOLTZMAN = 1.38064852;
//        double p = pow(M_E, (double) (*cost - next_cost) / temperature);
        double p = pow(M_E, (double) (*cost - next_cost) / temperature);
        debug("[%d] SA acceptance probability = %g (cost - next_cost = %d)", iter, p, *cost - next_cost);
        double r = rand_uniform(0, 1);
        if (r < p) {
            debug("[%d] SA accepted, moving to %d", iter, next_cost);
            neighbourhood_swap(sol, &m,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);
            *cost = next_cost;
        }
    }

    neighbourhood_swap_iter_destroy(&swap_iter);
}


void simulated_annealing_solver_config_init(simulated_annealing_solver_config *config) {
    config->idle_limit = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void simulated_annealing_solver_config_destroy(simulated_annealing_solver_config *config) {

}

void simulated_annealing_solver_init(simulated_annealing_solver *solver) {
    solver->error = NULL;
}

void simulated_annealing_solver_destroy(simulated_annealing_solver *solver) {
    free(solver->error);
}

const char *simulated_annealing_solver_get_error(simulated_annealing_solver *solver) {
    return solver->error;
}

void simulated_annealing_solver_reinit(simulated_annealing_solver *solver) {
    simulated_annealing_solver_destroy(solver);
    simulated_annealing_solver_init(solver);
}

static int compute_t0(solution * s) {
    neighbourhood_swap_iter swap_iter;
    neighbourhood_swap_iter_init(&swap_iter, s);

    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result;
    int best_swap_cost = INT_MAX;
    int worst_swap_cost = INT_MIN;

    while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
        neighbourhood_swap(s, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);
        if (swap_result.feasible) {
            if (swap_result.delta_cost < best_swap_cost) {
                best_swap_cost = swap_result.delta_cost;
            }
            if (swap_result.delta_cost > best_swap_cost) {
                worst_swap_cost = swap_result.delta_cost;
            }
        }
    }

    neighbourhood_swap_iter_destroy(&swap_iter);


    return worst_swap_cost - best_swap_cost;
}

bool simulated_annealing_solver_solve(simulated_annealing_solver *solver,
                               simulated_annealing_solver_config *config,
                               solution *s) {
    CRDSQT(s->model);
    const model *model = s->model;
    simulated_annealing_solver_reinit(solver);

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

    int t = compute_t0(s);
    double alpha = 0.9;

    int same_t_iter = 0;
    int T_KEEP = 10;

    debug("t0 = %d", t);

    neighbourhood_swap_move *swap_moves =
                mallocx(solution_assignment_count(s) * R * D * S,
                        sizeof(neighbourhood_swap_move));

    while (best_solution_cost > 0) {
        if (idle_limit > 0 && !(idle_iter < idle_limit) && t == 0) {
            verbose("Idle limit reached (%d), stopping here", idle_iter);
            break;
        }
        if (time_limit > 0 && !(ms() < time_limit)) {
            verbose("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }

        same_t_iter = (same_t_iter + 1) % T_KEEP;
        if (same_t_iter == 0) {
            t = (int) ((double) t * (alpha));
            alpha *= 1.005;
            alpha = RANGIFY(0.8, alpha, 0.99);
            if (t < 1)
                t = 0;
            debug("[%d] t0 decreased to %d", iter, t);
        }

        verbose("[%d] t=%d alpha=%g| cost=%d    // best is %d)", iter, t, alpha, cost, best_solution_cost);
        char tmp[64];
        snprintf(tmp, 64, "%d", cost);
//        fileappend("/tmp/trend.txt", tmp);

        assert(cost == solution_cost(s));

        do_simulated_annealing_phase(s, &cost, best_solution_cost, iter, t, swap_moves);

        // Check if the solution found is better than the best known
        if (cost < best_solution_cost) {
            verbose("[%d] SA: new best solution found, cost = %d", iter, cost);
            best_solution_cost = cost;
            idle_iter = 0;
        } else {
            idle_iter++;
        }

        iter++;
    }

//    solution_destroy(s);
    free(swap_moves);

    if (best_solution_cost == INT_MAX)
        solver->error = strmake("unable to find a feasible solution");

    return strempty(solver->error);
}
