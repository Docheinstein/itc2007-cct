#include "hill_climbing_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <utils/io_utils.h>
#include <neighbourhoods/neighbourhood_stabilize_room.h>
#include <utils/assert_utils.h>
#include <utils/mem_utils.h>
#include <utils/random_utils.h>
#include <math.h>
#include <utils/math_utils.h>
#include <utils/array_utils.h>
#include "neighbourhoods/neighbourhood_swap.h"
#include "feasible_solution_finder.h"
#include "renderer.h"

/*
        // Phase 2
#define PHASE2 1
#if PHASE2
        debug("[%d] Phase 2: hill_climbing_local_search_phase", state.cycle);
        hill_climbing_local_search_phase(&state,
                                         neighbourhood_swap_accept_less_cost, NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_step] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif

        // Phase 3
#define PHASE3 0
#if PHASE3
        debug("[%d] Phase 3: hill_climbing_local_search_depth_2_phase", state.cycle);
        hill_climbing_local_search_depth_2_phase(&state,
                                                 neighbourhood_swap_accept_less_or_equal_0,
                                                 neighbourhood_swap_accept2_less_or_equal_cost,
                                                 NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_depth_2_phase] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif

#define PHASE4 0
#if PHASE4
        const double MAX_TOL = .10;
        double tolerance = 1 + MAX_TOL - map((double) starting_time, (double) time_limit, 0, MAX_TOL, (double) ms());
//        debug("tol =%g", tolerance);
        hill_climbing_phase(s, &cost, best_solution_cost, neighbourhood_swap_accept_cost_tolerance, &tolerance, 100);
        debug("[%d] Cost after phase 2: %d      // best is %d", major_cycle, cost, best_solution_cost);

        if (cost < best_solution_cost) {
            verbose("[%d] New best solution found, cost = %d", major_cycle, cost);
            best_solution_cost = cost;
            solution_copy(&best_solution, s);
        }
#endif

        // Phase 1
#define PHASE5 0
#if PHASE5
        debug("[%d] Phase 3: hill_climbing_local_search_depth_2_phase", state.cycle);
        hill_climbing_local_search_depth_2_phase(&state,
                                                 neighbourhood_swap_accept_less_or_equal_0,
                                                 neighbourhood_swap_accept2_less_or_equal_cost,
                                                 NULL);
        debug("[%d] Cost after phase [hill_climbing_local_search_depth_2_phase] = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);
#endif
*/

void hill_climbing_solver_config_init(hill_climbing_solver_config *config) {
    config->idle_limit = 0;
    config->time_limit = 0;
    config->difficulty_ranking_randomness = 0;
    config->starting_solution = NULL;
}

void hill_climbing_solver_config_destroy(hill_climbing_solver_config *config) {

}

void hill_climbing_solver_init(hill_climbing_solver *solver) {
    solver->error = NULL;
}

void hill_climbing_solver_destroy(hill_climbing_solver *solver) {
    free(solver->error);
}

const char *hill_climbing_solver_get_error(hill_climbing_solver *solver) {
    return solver->error;
}

void hill_climbing_solver_reinit(hill_climbing_solver *solver) {
    hill_climbing_solver_destroy(solver);
    hill_climbing_solver_init(solver);
}

typedef struct hill_climbing_solver_state {
    solution *current_solution;
    solution *best_solution;

    int current_cost;
    int best_cost;
    int cycle;
} hill_climbing_solver_state;

typedef bool (*neighbourhood_swap_acceptor)(
        neighbourhood_swap_result /* swap result */,
        hill_climbing_solver_state *state,
        void * /* arg */);

typedef bool (*neighbourhood_swap_acceptor2)(
        neighbourhood_swap_result /* swap result 1*/,
        neighbourhood_swap_result /* swap result 2*/,
        hill_climbing_solver_state *state,
        void * /* arg */);


typedef bool (*hill_climber_step)(
        hill_climbing_solver_state *,
        neighbourhood_swap_acceptor,
        void *);

static void eventually_update_best_solution(hill_climbing_solver_state *state) {
    if (state->current_cost < state->best_cost) {
        state->best_cost = state->current_cost;
        solution_copy(state->best_solution, state->current_solution);
        verbose("[%d] Best solution found = %d", state->cycle, state->best_cost);
    }
}

static bool hill_climbing_local_search_depth_2_phase(
        hill_climbing_solver_state *state,
        neighbourhood_swap_acceptor accept1,
        neighbourhood_swap_acceptor2 accept2,
        void *acceptor_arg) {

    bool idling;

//    do {
//        idling = true;

    neighbourhood_swap_iter swap_iter1;
    neighbourhood_swap_iter_init(&swap_iter1, state->current_solution);
    neighbourhood_swap_move swap_mv1, swap_mv2;
    neighbourhood_swap_result swap_res1, swap_res2;

    while (neighbourhood_swap_iter_next(&swap_iter1, &swap_mv1)) {
        neighbourhood_swap(state->current_solution, &swap_mv1,
                           NEIGHBOURHOOD_PREDICT_ALWAYS,
                           NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                           NEIGHBOURHOOD_PREDICT_NEVER,
                           NEIGHBOURHOOD_PERFORM_NEVER,
                           &swap_res1);

        if (swap_res1.feasible && accept1(swap_res1, state, acceptor_arg)) {
            neighbourhood_swap(state->current_solution, &swap_mv1,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);
            state->current_cost += swap_res1.delta_cost;
            eventually_update_best_solution(state);

            // --- swap 2 ---
            neighbourhood_swap_iter swap_iter2;
            neighbourhood_swap_iter_init(&swap_iter2, state->current_solution);

            while (neighbourhood_swap_iter_next(&swap_iter2, &swap_mv2)) {
                debug2("hill_climbing_local_search_depth_2_phase %d.%d", swap_iter1.i, swap_iter2.i);

                neighbourhood_swap(state->current_solution, &swap_mv2,
                                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                                   NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                                   NEIGHBOURHOOD_PREDICT_NEVER,
                                   NEIGHBOURHOOD_PERFORM_NEVER,
                                   &swap_res2);
                if (swap_res2.feasible && accept2(swap_res1, swap_res2, state, acceptor_arg)) {
                    debug("Performing a double move of cost %d", swap_res1.delta_cost + swap_res2.delta_cost);
                    neighbourhood_swap(state->current_solution, &swap_mv2,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PREDICT_NEVER,
                                       NEIGHBOURHOOD_PERFORM_ALWAYS,
                                       &swap_res2);
                    state->current_cost += swap_res1.delta_cost + swap_res2.delta_cost;
                    eventually_update_best_solution(state);
                    idling = false;
                }
            }

            neighbourhood_swap_iter_destroy(&swap_iter2);

            // --------------
            if (!idling)
                break;

            neighbourhood_swap_back(state->current_solution, &swap_mv1, &swap_res1, false);
        }
    }
    neighbourhood_swap_iter_destroy(&swap_iter1);
//    } while (!idling);
}

static bool hill_climbing_local_search_phase(
        hill_climbing_solver_state *state,
        neighbourhood_swap_acceptor accept,
        void *acceptor_arg) {
    bool idling;
    do {
        idling = true;

        neighbourhood_swap_iter swap_iter;
        neighbourhood_swap_iter_init(&swap_iter, state->current_solution);

        neighbourhood_swap_move swap_mv;
        neighbourhood_swap_result swap_result;

        while (neighbourhood_swap_iter_next(&swap_iter, &swap_mv)) {
            neighbourhood_swap(state->current_solution, &swap_mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &swap_result);

            if (swap_result.feasible && accept(swap_result, state, acceptor_arg)) {
                neighbourhood_swap(state->current_solution, &swap_mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   NULL);
                state->current_cost += swap_result.delta_cost;
                eventually_update_best_solution(state);
                idling = false;
                break;
            }
        }

        neighbourhood_swap_iter_destroy(&swap_iter);
    } while (!idling);
}

static bool hill_climbing_neighbourhood_stabilize_room_phase(hill_climbing_solver_state *state) {
    bool idling;
    do {
        idling = true;

        neighbourhood_stabilize_room_iter stabilize_room_iter;
        neighbourhood_stabilize_room_iter_init(&stabilize_room_iter, state->current_solution);

        neighbourhood_stabilize_room_move stabilize_room_mv;
        neighbourhood_stabilize_room_result stabilize_room_result;

        while (neighbourhood_stabilize_room_iter_next(&stabilize_room_iter, &stabilize_room_mv)) {
            neighbourhood_stabilize_room(state->current_solution, &stabilize_room_mv,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &stabilize_room_result);

            if (stabilize_room_result.delta_cost < 0) {
                neighbourhood_stabilize_room(state->current_solution, &stabilize_room_mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   NULL);
                state->current_cost += stabilize_room_result.delta_cost;
                eventually_update_best_solution(state);
                idling = false;
                break;
            }
        }

        neighbourhood_stabilize_room_iter_destroy(&stabilize_room_iter);
    } while (!idling);
}

bool hill_climbing_try_random_step(hill_climbing_solver_state *state,
                                   neighbourhood_swap_acceptor accept,
                                   void *acceptor_arg) {
    CRDSQT(state->current_solution->model);

    neighbourhood_swap_move swap_mv;
    neighbourhood_swap_result swap_result;

    neighbourhood_swap_generate_random_move(state->current_solution, &swap_mv);
    neighbourhood_swap(state->current_solution, &swap_mv,
                       NEIGHBOURHOOD_PREDICT_ALWAYS,
                       NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                       NEIGHBOURHOOD_PREDICT_NEVER,
                       NEIGHBOURHOOD_PERFORM_NEVER,
                       &swap_result);
    if (!swap_result.feasible || !accept(swap_result, state, acceptor_arg))
        return false; // doing nothing

    neighbourhood_swap(state->current_solution, &swap_mv,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);

    state->current_cost += swap_result.delta_cost;
    eventually_update_best_solution(state);

    return true;
}

void hill_climbing_random_phase(hill_climbing_solver_state *state,
                                hill_climber_step step,
                                int max_performed_step, int max_step_idle,
                                neighbourhood_swap_acceptor accept,
                                void * acceptor_arg) {
    int performed = 0;
    int idle = 0;
    while (idle <= max_step_idle && performed <= max_performed_step) {
        int prev_cost = state->current_cost;
        performed += step(state, accept, acceptor_arg);
        idle = state->current_cost < prev_cost ? 0 : idle + 1;
    }
}

static bool neighbourhood_swap_accept_less_cost(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return state->current_cost + result.delta_cost < state->best_cost;
}

static bool neighbourhood_swap_accept_always(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return true;
}

static bool neighbourhood_swap_accept_less_or_equal_0(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept2_less_or_equal_cost(neighbourhood_swap_result result1, neighbourhood_swap_result result2,
                                                 hill_climbing_solver_state *state, void *arg) {
    return state->current_cost + result1.delta_cost + result2.delta_cost < state->best_cost;
}

static bool neighbourhood_swap_accept_cost_tolerance(neighbourhood_swap_result result, hill_climbing_solver_state *state, void *arg) {
    double tolerance = *((double *) arg);
    return state->current_cost + result.delta_cost <= state->best_cost * tolerance;
}

static bool neighbourhood_swap_accept_less_or_equal_cost(neighbourhood_swap_result result,
                                                         hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rc_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_capacity <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_cc_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_curriculum_compactness <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_mwd_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_min_working_days <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rs_cost(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    return result.delta_cost_room_stability <= 0;
}

static bool neighbourhood_swap_accept_less_or_equal_rc_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_room_capacity <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_cc_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_curriculum_compactness <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_min_working_days <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static bool neighbourhood_swap_accept_less_or_equal_rs_cost_bounded(neighbourhood_swap_result result,
                                                            hill_climbing_solver_state *state, void *arg) {
    int max_allowed = *((int *) arg);
    return result.delta_cost_room_stability <= 0 && state->current_cost + result.delta_cost < max_allowed;
}

static neighbourhood_swap_acceptor random_acceptor() {
    static neighbourhood_swap_acceptor acceptors[] = {
        neighbourhood_swap_accept_less_or_equal_cost,
        neighbourhood_swap_accept_less_or_equal_rc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_rs_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_cc_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
        neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded,
    };

    return acceptors[rand_int_range(0, LENGTH(acceptors))];
}

bool hill_climbing_solver_solve(hill_climbing_solver *solver,
                               hill_climbing_solver_config *config,
                               solution *s) {
    hill_climbing_solver_reinit(solver);

    long starting_time = ms();
    long time_limit = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;
    int idle_limit = config->idle_limit;

    if (config->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(s, config->starting_solution);
    }
    else {
        debug("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

        feasible_solution_finder_find(&finder, &finder_config, s);
    }

    solution best_solution;
    solution_init(&best_solution, s->model);
    solution_copy(&best_solution, s);

    int initial_cost = solution_cost(s);

    hill_climbing_solver_state state = {
        .current_solution = s,
        .current_cost = initial_cost,
        .best_solution = &best_solution,
        .best_cost = initial_cost,
    };

    state.cycle = 0;

    while (true) {
        if (time_limit > 0 && !(ms() < time_limit)) {
            verbose("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }
//        assert(state.current_cost == solution_cost(state.current_solution));

        debug("[%d] -------- MAJOR CYCLE BEGIN --------", state.cycle);
        debug("[%d] Cost = %d                   // best = %d",
              state.cycle, state.current_cost, state.best_cost);

        // Phase 1
        debug("[%d] PHASE 1 - HC - Swap move", state.cycle);
        hill_climbing_random_phase(&state,
                            hill_climbing_try_random_step, INT_MAX, idle_limit,
                            neighbourhood_swap_accept_less_or_equal_cost, NULL);
        debug("[%d] PHASE 1 END, cost = %d      // best = %d",
              state.cycle, state.current_cost, state.best_cost);

        // Phase 1.5
        double r = rand_uniform(0, 1);
        if (r < 0.1) {
#define PHASE1_5 1
#if PHASE1_5
            debug("[%d] Phase 1.5: hill_climbing_neighbourhood_stabilize_room_phase", state.cycle);
            hill_climbing_neighbourhood_stabilize_room_phase(&state);
            debug("[%d] Cost after phase [hill_climbing_neighbourhood_stabilize_room_phase] = %d      // best = %d",
                  state.cycle, state.current_cost, state.best_cost);
#endif
        } else {
            // Phase 2
            neighbourhood_swap_acceptor acceptor = random_acceptor();

            static const int UNCHECKED_MOVES_UB = 30;
            static const int UNCHECKED_MOVES_LB = 5;

            double max_allowed_cost_factor =
                    map((double) starting_time, (double) time_limit, 2, 1.3, (double) ms());
            int max_allowed_cost = (int) (state.current_cost * max_allowed_cost_factor);

            int n_moves = (int) map((double) starting_time, (double) time_limit, UNCHECKED_MOVES_UB, UNCHECKED_MOVES_LB,
                                    (double) ms());
            debug("[%d] PHASE 2 - HC - %s move (n_moves=%d, max_allowed_cost=%d)", state.cycle,
                  (acceptor == neighbourhood_swap_accept_less_or_equal_rc_cost_bounded ? "RoomCapacity" :
                   (acceptor == neighbourhood_swap_accept_less_or_equal_mwd_cost_bounded ? "MinWorkingDays" :
                    (acceptor == neighbourhood_swap_accept_less_or_equal_cc_cost_bounded ? "CurriculumCompactness" :
                     (acceptor == neighbourhood_swap_accept_less_or_equal_rs_cost_bounded ? "RoomStability" :
                      (acceptor == neighbourhood_swap_accept_less_or_equal_cost ? "Swap" : "Unknown"))))),
                  n_moves, max_allowed_cost);
            hill_climbing_random_phase(&state,
                                       hill_climbing_try_random_step, n_moves, n_moves * 10,
                                       acceptor, &max_allowed_cost);
            debug("[%d] PHASE 2 END, cost = %d      // best = %d",
                  state.cycle, state.current_cost, state.best_cost);
        }
        state.cycle++;
    }

    solution_copy(s, &best_solution);

    return strempty(solver->error);
}
