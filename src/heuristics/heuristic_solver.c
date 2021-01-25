#include "heuristic_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include "feasible_solution_finder.h"
#include "utils/mem_utils.h"


void heuristic_solver_config_init(heuristic_solver_config *config) {
    config->time_limit = -1;
    config->cycles_limit = -1;
    config->starting_solution = NULL;
    config->multistart = false;
    config->methods = g_array_new(false, false, sizeof(heuristic_method_parameterized));
}

void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_method method,
                                        void *param,
                                        const char *name) {
    heuristic_method_parameterized method_parameterized = {
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
                                          heuristic_solver_state *state) {
    if (solver_conf->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(state->current_solution, solver_conf->starting_solution);
    }
    else if (state->current_cost == INT_MAX || solver_conf->multistart) {
        debug("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
//        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

        if (state->current_cost != INT_MAX)
            solution_reinit(state->current_solution);
        feasible_solution_finder_find(&finder, &finder_config, state->current_solution);
        // error, in case in unable to find solution

        state->current_cost = solution_cost(state->current_solution);
        heuristic_solver_state_update(state);

        verbose("Feasible initial solution found, cost = %d", state->current_cost);
    }

    return true;
}

bool heuristic_solver_solve(heuristic_solver *solver, const heuristic_solver_config *solver_conf,
                            solution *sol_out) {
    const model *model = sol_out->model;

    heuristic_method_parameterized *methods = (heuristic_method_parameterized *) solver_conf->methods->data;
    int n_methods = solver_conf->methods->len;

    if (!n_methods) {
        solver->error = strmake("no methods provided to solver");
        return false;
    }

    long starting_time = ms();
    long deadline = solver_conf->time_limit > 0 ? (starting_time + solver_conf->time_limit * 1000) : 0;
    int cycles_limit = solver_conf->cycles_limit >= 0 ? solver_conf->cycles_limit : INT_MAX;

    solution current_solution;
    solution_init(&current_solution, model);

    heuristic_solver_state state = {
        .model = model,
        .current_solution = &current_solution,
        .current_cost = INT_MAX,
        .best_solution = sol_out,
        .best_cost = INT_MAX
    };

    if (get_verbosity()) {
        char *names[n_methods];
        for (int i = 0; i < n_methods; i++)
            names[i] = strdup(methods[i].name);
        char *methods_str = strjoin(names, n_methods, ", ");
        verbose("Starting solver with %d methods (%s)", n_methods, methods_str);
        for (int i = 0; i < n_methods; i++)
            free(names[i]);
        free(methods_str);
    }

    while (state.best_cost > 0) {
        if (deadline > 0 && !(ms() < deadline)) {
            debug("Time limit reached (%ds), stopping here", solver_conf->time_limit);
            break;
        }

        if (!(state.cycle < cycles_limit)) {
            debug("Cycles limit reached (%d), stopping here", cycles_limit);
            break;
        }

        // In case of multistart, generate a new solution each cycle
        generate_feasible_solution_if_needed(solver_conf, &state);

        verbose("============ CYCLE %d ============\n"
                "Current = %d\n"
                "Best = %d\n"
                "Cost relative to best = %g%%",
                state.cycle, state.current_cost, state.best_cost,
                (double) 100 * state.current_cost / state.best_cost);
//        verbose("[%d] Non increasing iters = %d",
//                state.cycle, non_increasing_iters);
//        verbose("[%d] Non best iters = %d",
//                state.cycle, non_best_iters);


        for (int i = 0; i < solver_conf->methods->len; i++) {
            heuristic_method_parameterized *method = &methods[i];
            verbose("-------- %s BEGIN --------", method->name);
            method->method(&state, method->param);
            verbose("-------- %s END --------", method->name);
        }

        state.cycle++;
    }

    solution_destroy(state.current_solution);

    return strempty(solver->error);
}

bool heuristic_solver_state_update(heuristic_solver_state *state) {
    if (state->current_cost < state->best_cost) {
        state->best_cost = state->current_cost;
        solution_copy(state->best_solution, state->current_solution);
        verbose("New best solution of cost %d", state->best_cost);
        return true;
    }
    return false;
}
