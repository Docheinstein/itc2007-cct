#include "heuristic_solver.h"
#include <log/debug.h>
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include "feasible_solution_finder.h"
#include "utils/mem_utils.h"


void heuristic_solver_config_init(heuristic_solver_config *config) {
    config->time_limit = 0;
    config->starting_solution = NULL;
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

bool heuristic_solver_solve(heuristic_solver *solver, const heuristic_solver_config *config,
                            solution *sol_out) {
    const model *model = sol_out->model;

    heuristic_method_parameterized *methods = (heuristic_method_parameterized *) config->methods->data;
    int n_methods = config->methods->len;

    if (!n_methods) {
        solver->error = strmake("no methods provided to solver");
        return false;
    }

    long starting_time = ms();
    long deadline = config->time_limit > 0 ? (starting_time + config->time_limit * 1000) : 0;

    solution current_solution;
    solution_init(&current_solution, model);

    if (config->starting_solution) {
        debug("Using provided initial solution");
        solution_copy(&current_solution, config->starting_solution);
    }
    else {
        debug("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        feasible_solution_finder_config finder_config;
        feasible_solution_finder_config_init(&finder_config);
//        finder_config.difficulty_ranking_randomness = config->difficulty_ranking_randomness;

        feasible_solution_finder_find(&finder, &finder_config, &current_solution);
        // error, in case in unable to find solution
    }

    solution_copy(sol_out, &current_solution);

    int initial_cost = solution_cost(&current_solution);

    heuristic_solver_state state = {
        .current_solution = &current_solution,
        .current_cost = initial_cost,
        .best_solution = sol_out,
        .best_cost = initial_cost,
    };

    verbose("Feasible initial solution found, cost = %d", initial_cost);
    // cost, attempts, ...

    char *names[n_methods];
    for (int i = 0; i < n_methods; i++)
        names[i] = strdup(methods[i].name);
    char *methods_str = strjoin(names, n_methods, ", ");
    verbose("Starting solver with %d methods (%s)", n_methods, methods_str);
    for (int i = 0; i < n_methods; i++)
        free(names[i]);
    free(methods_str);

    while (state.best_cost > 0) {
        if (deadline > 0 && !(ms() < deadline)) {
            debug("Time limit reached (%ds), stopping here", config->time_limit);
            break;
        }

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

        for (int i = 0; i < config->methods->len; i++) {
            heuristic_method_parameterized *method = &methods[i];
            verbose("-------- %s --------",
                    method->name);
            method->method(&state, method->param);
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
