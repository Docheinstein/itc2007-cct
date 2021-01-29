#include "heuristic_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <stdlib.h>
#include <utils/assert_utils.h>
#include <renderer/renderer.h>
#include "timeout/timeout.h"
#include "finder/feasible_solution_finder.h"
#include "utils/mem_utils.h"


void heuristic_solver_config_init(heuristic_solver_config *config) {
    config->methods = g_array_new(false, false, sizeof(heuristic_solver_method_callback_param));
    config->max_time = -1;
    config->max_cycles = -1;
    config->starting_solution = NULL;
    config->multistart = false;
    config->restore_best_after_cycles = -1;
    config->dont_solve = false;
}

void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_solver_method_callback method,
                                        void *param,
                                        const char *name) {
    heuristic_solver_method_callback_param method_parameterized = {
        .method = method,
        .param = param,
        .name = name
    };
    g_array_append_val(config->methods, method_parameterized);
}

void heuristic_solver_config_destroy(heuristic_solver_config *config) {
    g_array_free(config->methods, true);
}

void heuristic_solver_init(heuristic_solver *solver) {
    solver->error = NULL;
}

void heuristic_solver_destroy(heuristic_solver *solver) {
    free(solver->error);
}

const char *heuristic_solver_get_error(heuristic_solver *solver) {
    return solver->error;
}

bool generate_feasible_solution_if_needed(const heuristic_solver_config *solver_conf,
                                          const feasible_solution_finder_config *finder_conf,
                                          heuristic_solver_state *state) {
    if (solver_conf->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(state->current_solution, solver_conf->starting_solution);
        return true;
    }

    if (state->current_cost == INT_MAX || solver_conf->multistart) {
        verbose("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        solution_clear(state->current_solution);
        if (feasible_solution_finder_find(&finder, finder_conf, state->current_solution)) {
            state->current_cost = solution_cost(state->current_solution);
            verbose("Found initial feasible solution of cost = %d", state->current_cost);
            heuristic_solver_state_update(state);
            return true;
        }
        else {
            return false;
        }
    }

    // Nothing to generate, continue from the current solution (standard case)
    return true;
}

bool heuristic_solver_solve(heuristic_solver *solver,
                            const heuristic_solver_config *solver_conf,
                            const feasible_solution_finder_config *finder_conf,
                            solution *sol_out) {
    const model *model = sol_out->model;

    heuristic_solver_method_callback_param *methods =
            (heuristic_solver_method_callback_param *) solver_conf->methods->data;
    int n_methods = solver_conf->methods->len;


    if (get_verbosity()) {
        char *names[n_methods];
        for (int i = 0; i < n_methods; i++)
            names[i] = strdup(methods[i].name);
        char *methods_str = strjoin(names, n_methods, ", ");
        verbose("solver.methods = %s", methods_str);
        for (int i = 0; i < n_methods; i++)
            free(names[i]);
        free(methods_str);

        verbose("solver.time_limit = %d", solver_conf->max_time);
        verbose("solver.cycles_limit = %d", solver_conf->max_cycles);
        verbose("solver.multistart = %s", booltostr(solver_conf->multistart));
        verbose("solver.restore_best_after_cycles = %d", solver_conf->restore_best_after_cycles);
    }

    if (solver_conf->max_time >= 0)
        set_timeout(solver_conf->max_time);

    int cycles_limit = solver_conf->max_cycles >= 0 ? solver_conf->max_cycles : INT_MAX;

    if (solver_conf->dont_solve)
        cycles_limit = 0;

    solution current_solution;
    solution_init(&current_solution, model);
    
    heuristic_solver_state *state = &solver->state;
    state->model = model;
    state->current_solution = &current_solution;
    state->current_cost = INT_MAX;
    state->best_solution = sol_out;
    state->best_cost = INT_MAX;
    state->cycle = 0;
    state->starting_time = ms();
    state->best_solution_time = LONG_MAX;
    state->ending_time = LONG_MAX;

    if (!n_methods) {
        solver->error = strmake("no methods provided to solver");
        return false;
    }

    int non_improving_best_cycles = 0;
    int non_improving_current_cycles = 0;

    while (state->best_cost > 0) {
        // In case of multistart, generate a new solution each cycle
        if (!generate_feasible_solution_if_needed(solver_conf, finder_conf, state))
            break;

        if (timeout) {
            debug("Time limit reached (%ds), stopping here", solver_conf->max_time);
            break;
        }

        if (!(state->cycle < cycles_limit)) {
            debug("Cycles limit reached (%d), stopping here", cycles_limit);
            break;
        }

        int cycle_begin_best_cost = state->best_cost;
        int cycle_begin_current_cost = state->current_cost;

        // Restore the best known solution after 'restore_best_after_cycles'
        if (solver_conf->restore_best_after_cycles > 0 &&
            non_improving_best_cycles == solver_conf->restore_best_after_cycles) {
            verbose("==============================================\n"
                    "Restoring best solution of cost %d after %d cycles not improving best",
                    state->best_cost, non_improving_best_cycles);
            solution_copy(state->current_solution, state->best_solution);
            state->current_cost = state->best_cost;
            non_improving_best_cycles = 0;
            non_improving_current_cycles = 0;
        }

        verbose("================== CYCLE %d ==================\n"
                "Current = %d\n"
                "Best = %d\n"
                "Cost relative to best = %g%%\n"
                "Not improving local best since %d cycles\n"
                "Not improving global best since %d cycles",
                state->cycle, state->current_cost, state->best_cost,
                (double) 100 * state->current_cost / state->best_cost,
                non_improving_current_cycles, non_improving_best_cycles);


        for (int i = 0; i < solver_conf->methods->len; i++) {
            heuristic_solver_method_callback_param *method = &methods[i];
            verbose("------------ %s BEGIN (%d) ------------",
                    method->name, state->current_cost);
            method->method(state, method->param);
            verbose("------------ %s END   (%d) --------------",
                    method->name, state->current_cost);
        }

        non_improving_current_cycles =
                (state->current_cost < cycle_begin_current_cost) ?
                0 : (non_improving_current_cycles + 1);
        non_improving_best_cycles =
                (state->best_cost < cycle_begin_best_cost) ?
                0 : (non_improving_best_cycles + 1);

        state->cycle++;
    }

    state->ending_time = ms();

    solution_destroy(state->current_solution);

    return strempty(solver->error);
}

bool heuristic_solver_state_update(heuristic_solver_state *state) {
    if (state->current_cost < state->best_cost) {
//        write_solution(state->best_solution, "/tmp/before");
//        render_solution_overview(state->best_solution, "/tmp/before.png");
//        write_solution(state->current_solution, "/tmp/after");
//        render_solution_overview(state->current_solution, "/tmp/after.png");
        verbose("Found new best solution of cost %d", state->current_cost);
        assert(solution_satisfy_hard_constraints(state->current_solution));
        assert(state->current_cost == solution_cost(state->current_solution));
        state->best_cost = state->current_cost;
        state->best_solution_time = ms();
        solution_copy(state->best_solution, state->current_solution);
        assert(state->best_cost == solution_cost(state->best_solution));
        return true;
    }
    return false;
}
