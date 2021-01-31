#include "heuristic_solver.h"
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/time_utils.h>
#include <stdlib.h>
#include <utils/assert_utils.h>
#include <utils/io_utils.h>
#include "timeout/timeout.h"
#include "finder/feasible_solution_finder.h"
#include "utils/mem_utils.h"

void heuristic_solver_config_init(heuristic_solver_config *config) {
    config->methods = g_array_new(false, false, sizeof(heuristic_solver_method_callback_parameterized));
    config->max_time = 60;
    config->max_cycles = -1;
    config->multistart = false;
    config->restore_best_after_cycles = 15;

    config->starting_solution = NULL;
    config->dont_solve = false;
}

void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_solver_method_callback method,
                                        void *param,
                                        const char *name) {
    heuristic_solver_method_callback_parameterized method_parameterized = {
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
        solution_copy(state->current_solution, solver_conf->starting_solution);
        return true;
    }

    if (state->current_cost == INT_MAX || solver_conf->multistart) {
        verbose2("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        solution_clear(state->current_solution);
        if (feasible_solution_finder_find(&finder, finder_conf, state->current_solution)) {
            state->current_cost = solution_cost(state->current_solution);
            verbose2("Found initial feasible solution of cost = %d", state->current_cost);
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

    heuristic_solver_method_callback_parameterized *methods =
            (heuristic_solver_method_callback_parameterized *) solver_conf->methods->data;
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

    if (!n_methods) {
        solver->error = strmake("no methods provided to solver");
        return false;
    }

    if (solver_conf->max_time >= 0)
        set_timeout(solver_conf->max_time);

    int cycles_limit = solver_conf->max_cycles >= 0 ? solver_conf->max_cycles : INT_MAX;

    solution current_solution;
    solution_init(&current_solution, model);

    heuristic_solver_state *state = &solver->state;
    state->model = model;
    state->current_solution = &current_solution;
    state->current_cost = INT_MAX;
    state->best_solution = sol_out;
    state->best_cost = INT_MAX;
    state->cycle = 0;
    state->method = 0;
    state->non_improving_best_cycles = 0;
    state->non_improving_current_cycles = 0;

    state->collect_stats = get_verbosity() >= 1;
    state->collect_trend = get_verbosity() >= 2;

    state->methods_name = mallocx(n_methods, sizeof(const char *));
    for (int i = 0; i < n_methods; i++)
        state->methods_name[i] = methods[i].name;

    if (state->collect_stats) {
            state->stats.move_count = 0;
        state->stats.methods = mallocx(n_methods, sizeof(heuristic_solver_state_method_stats));
        for (int i = 0; i < n_methods; i++) {
            state->stats.methods[i].improvement = 0;
            state->stats.methods[i].execution_time = 0;
            state->stats.methods[i].move_count = 0;
        }
        state->stats.starting_time = ms();
        state->stats.best_solution_time = LONG_MAX;
        state->stats.ending_time = LONG_MAX;
        state->stats.last_log_time = 0;
        state->stats.trend_current = g_array_new(false, false, sizeof(int));
        state->stats.trend_best = g_array_new(false, false, sizeof(int));
    }

    long now;

    while (state->best_cost > 0) {
        if (timeout) {
            verbose("Time limit reached (%ds), stopping here", solver_conf->max_time);
            break;
        }

        if (!(state->cycle < cycles_limit)) {
            verbose("Cycles limit reached (%d), stopping here", cycles_limit);
            break;
        }
        // In case of multistart, generate a new solution each cycle
        if (!generate_feasible_solution_if_needed(solver_conf, finder_conf, state))
            break;

        if (solver_conf->dont_solve)
            break;

        int cycle_begin_best_cost = state->best_cost;
        int cycle_begin_current_cost = state->current_cost;

        // Restore the best known solution after 'restore_best_after_cycles'
        if (solver_conf->restore_best_after_cycles > 0 && !solver_conf->multistart &&
            state->non_improving_best_cycles >= solver_conf->restore_best_after_cycles) {
            verbose("Restoring best solution of cost %d after %d cycles not improving best",
                    state->best_cost, state->non_improving_best_cycles);
            solution_copy(state->current_solution, state->best_solution);
            state->current_cost = state->best_cost;
            state->non_improving_best_cycles = 0;
            state->non_improving_current_cycles = 0;
        }

        if (state->collect_stats) {
            int lv = 2;
            if (get_verbosity() < lv) {
                now = clk();
                if (now - state->stats.last_log_time > 1000) {
                    state->stats.last_log_time = now;
                    lv = 1;
                }
            }

            verbose_lv(lv,
                       "================== CYCLE %d ==================\n"
                       "Current = %d (%.2f%% of best)\n"
                       "Best = %d\n"
                       "Not improving local best since %d cycles\n"
                       "Not improving global best since %d cycles\n"
                       "Cycles = %d (cycles/s = %.2f)\n"
                       "Moves = %d (moves/s = %.2f)\n",
                       state->cycle, state->current_cost,
                       (double) 100 * state->current_cost / state->best_cost,
                       state->best_cost,
                       state->non_improving_current_cycles,
                       state->non_improving_best_cycles,
                       state->cycle,
                       (double) 100 * (double) state->cycle / (double) (ms() - state->stats.starting_time),
                       state->stats.move_count,
                       (double) 100 * (double) state->stats.move_count / (double) (ms() - state->stats.starting_time)
           );
        }


        for (int i = 0; i < solver_conf->methods->len; i++) {
            heuristic_solver_method_callback_parameterized *method = &methods[i];
            verbose2("------------ %s BEGIN (%d) ------------",
                    method->name, state->current_cost);
            state->method = i;
            if (state->collect_stats)
                now = clk();
            method->method(state, method->param);
            if (state->collect_stats)
                state->stats.methods[i].execution_time += (clk() - now);
            if (state->collect_trend) {
                g_array_append_val(state->stats.trend_current, state->current_cost);
                g_array_append_val(state->stats.trend_best, state->best_cost);
            }
            verbose2("------------ %s END   (%d) --------------",
                    method->name, state->current_cost);
        }

        state->non_improving_current_cycles =
                (state->current_cost < cycle_begin_current_cost) ?
                0 : (state->non_improving_current_cycles + 1);
        state->non_improving_best_cycles =
                (state->best_cost < cycle_begin_best_cost) ?
                0 : (state->non_improving_best_cycles + 1);

        state->cycle++;
    }

    if (state->collect_stats) {
        state->stats.ending_time = ms();

        verbose("=============== SOLVER FINISHED ===============\n"
                "Best = %d (found after %.2f seconds)\n"
                "Execution time = %.2f seconds\n"
                "Cycles = %d (cycles/s = %.2f)\n"
                "Moves = %d (moves/s = %.2f)",
                state->best_cost,
                (double) (state->stats.best_solution_time - state->stats.starting_time) / 1000,
                (double) (state->stats.ending_time - state->stats.starting_time) / 1000,
                state->cycle,
                (double) 100 * (double) state->cycle / (double) (ms() - state->stats.starting_time),
                state->stats.move_count,
                (double) 100 * (double) state->stats.move_count / (double) (ms() - state->stats.starting_time)
        );

        for (int i = 0; i < n_methods; i++) {
            verbose("%s:\n"
                    "   Execution time = %.2f seconds\n"
                    "   Moves = %d (moves/s = %.2f)\n"
                    "   Overall improvement = %d",
                    state->methods_name[i],
                    (double) state->stats.methods[i].execution_time / 1000,
                    state->stats.methods[i].move_count,
                    (double) 100 * (double) state->stats.methods[i].move_count /
                        (double) (state->stats.methods[i].execution_time),
                    state->stats.methods[i].improvement);
        }

        if (state->collect_trend) {
            verbose2("--- TREND OF CURRENT ---");
            int *trend_current = (int *) state->stats.trend_current->data;
            for (int i = 0; i < state->stats.trend_current->len; i++) {
                if (i && i % 10 == 0)
                    verbosef2("\n");
                verbosef2("%5d", trend_current[i]);
            }
            verbosef2("\n");

            verbose2("--- TREND OF BEST ---");
            int *trend_best = (int *) state->stats.trend_best->data;
            for (int i = 0; i < state->stats.trend_best->len; i++) {
                if (i && i % 10 == 0)
                    verbosef2("\n");
                verbosef2("%5d", trend_best[i]);
            }
            verbosef2("\n");
        }

        free(state->stats.methods);
        g_array_free(state->stats.trend_current, true);
        g_array_free(state->stats.trend_best, true);
    }

    free(state->methods_name);
    solution_destroy(state->current_solution);

    return strempty(solver->error);
}

bool heuristic_solver_state_update(heuristic_solver_state *state) {
    bool improved = false;

    if (state->current_cost < state->best_cost) {
        verbose("%s: found new best solution of cost %d",
                state->methods_name[state->method], state->current_cost);
        assert(solution_satisfy_hard_constraints(state->current_solution));
        if (state->collect_stats) {
            if (state->best_cost < INT_MAX)
                state->stats.methods[state->method].improvement += (state->best_cost - state->current_cost);
            state->stats.best_solution_time = ms();
        }

        state->best_cost = state->current_cost;

        solution_copy(state->best_solution, state->current_solution);
        assert(state->best_cost == solution_cost(state->best_solution));
        improved = true;

//        if (state->collect_trend) {
//            g_array_append_val(state->stats.trend_current, state->current_cost);
//            g_array_append_val(state->stats.trend_best, state->best_cost);
//        }
    }

    if (state->collect_stats) {
        state->stats.move_count++;
        state->stats.methods[state->method].move_count++;
    }

   return improved;
}
