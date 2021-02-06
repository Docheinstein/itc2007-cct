#include "heuristic_solver.h"
#include <stdlib.h>
#include <stdio.h>
#include <config/config.h>
#include "log/verbose.h"
#include "utils/str_utils.h"
#include "utils/time_utils.h"
#include "utils/assert_utils.h"
#include "utils/mem_utils.h"
#include "timeout/timeout.h"
#include "finder/feasible_solution_finder.h"

void heuristic_solver_config_init(heuristic_solver_config *config) {
    config->methods = g_array_new(false, false, sizeof(heuristic_solver_method_callback_parameterized));
    config->max_time = 60;
    config->max_cycles = -1;
    config->multistart = false;
    config->restore_best_after_cycles = 50;

    config->starting_solution = NULL;
    config->dont_solve = false;
    config->new_best_callback.callback = NULL;
    config->new_best_callback.arg = NULL;
}

void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_solver_method_callback method,
                                        void *param,
                                        const char *name,
                                        const char *short_name) {
    heuristic_solver_method_callback_parameterized method_parameterized = {
        .method = method,
        .param = param,
        .name = name,
        .short_name = short_name
    };
    g_array_append_val(config->methods, method_parameterized);
}

void heuristic_solver_config_destroy(heuristic_solver_config *config) {
    g_array_free(config->methods, true);
}


void heuristic_solver_stats_init(heuristic_solver_stats *stats) {
    stats->cycle_count = 0;
    stats->move_count = 0;
    stats->best_restored_count = 0;
    stats->methods = NULL;
    stats->starting_time = LONG_MAX;
    stats->best_solution_time = LONG_MAX;
    stats->ending_time = LONG_MAX;
}

void heuristic_solver_stats_destroy(heuristic_solver_stats *stats) {
    if (stats->methods) {
        for (int m = 0; m < stats->n_methods; m++) {
            g_array_free(stats->methods[m].trend.current.before, true);
            g_array_free(stats->methods[m].trend.current.after, true);
            g_array_free(stats->methods[m].trend.best.before, true);
            g_array_free(stats->methods[m].trend.best.after, true);
        }
        free(stats->methods);
    }
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

/* Generate a solution if needed (i.e. first time or if multistart=true) */
static bool generate_feasible_solution_if_needed(
        const heuristic_solver_config *solver_conf,
        const feasible_solution_finder_config *finder_conf,
        heuristic_solver_state *state) {
    if (state->current_cost != INT_MAX && !solver_conf->multistart)
        // Nothing to generate
        return true;

    if (solver_conf->starting_solution) {
        // If a starting solution is given, start always from it
        verbose2("Starting from loaded solution...");
        solution_copy(state->current_solution, solver_conf->starting_solution);
    } else {
        // Generate a new initial solution
        verbose2("Finding initial feasible solution...");
        feasible_solution_finder finder;
        feasible_solution_finder_init(&finder);

        solution_clear(state->current_solution);

        if (!feasible_solution_finder_find(&finder, finder_conf, state->current_solution))
            // Cannot find feasible solution (probably timed-out)
            return false;
    }

    // A new feasible solution have been generated
    state->current_cost = solution_cost(state->current_solution);
    verbose2("Starting from solution of cost = %d", state->current_cost);

    heuristic_solver_state_update(state);
    return true;
}

bool heuristic_solver_solve(heuristic_solver *solver,
                            const heuristic_solver_config *solver_conf,
                            const feasible_solution_finder_config *finder_conf,
                            solution *sol_out,
                            heuristic_solver_stats *statistics) {
    const model *model = sol_out->model;

    heuristic_solver_method_callback_parameterized *methods =
            (heuristic_solver_method_callback_parameterized *) solver_conf->methods->data;
    int n_methods = solver_conf->methods->len;

    if (get_verbosity()) {
        char *names[n_methods];
        for (int i = 0; i < n_methods; i++)
            names[i] = strdup(methods[i].name);
        char *methods_str = strjoin(names, n_methods, ", ");
        for (int i = 0; i < n_methods; i++)
            free(names[i]);

        verbose("solver.methods = %s", methods_str);
        verbose("solver.time_limit = %d", solver_conf->max_time);
        verbose("solver.cycles_limit = %d", solver_conf->max_cycles);
        verbose("solver.multistart = %s", booltostr(solver_conf->multistart));
        verbose("solver.restore_best_after_cycles = %d", solver_conf->restore_best_after_cycles);

        free(methods_str);
    }

    if (!n_methods) {
        solver->error = strmake("no methods provided to solver");
        return false;
    }

    if (solver_conf->max_time >= 0)
        set_timeout(solver_conf->max_time);

    int cycles_limit = solver_conf->max_cycles >= 0 ? solver_conf->max_cycles : INT_MAX;
    bool collect_trend = get_verbosity();

    solution current_solution;
    solution_init(&current_solution, model);

    // Initialize solver's state
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
    state->_last_log_time = 0;
    state->config = solver_conf;
    state->stats = statistics;

    state->methods_name = mallocx(n_methods, sizeof(const char *));
    state->stats->methods = mallocx(n_methods, sizeof(heuristic_solver_state_method_stats));
    state->stats->n_methods = n_methods;

    for (int i = 0; i < n_methods; i++) {
        state->methods_name[i] = methods[i].name;

        state->stats->methods[i].execution_time = 0;
        state->stats->methods[i].move_count = 0;
        state->stats->methods[i].improvement_count = 0;
        state->stats->methods[i].improvement_count_after_first_cycle = 0;
        state->stats->methods[i].improvement_delta = 0;
        state->stats->methods[i].improvement_delta_after_first_cycle = 0;
        state->stats->methods[i].trend.current.before = g_array_new(false, false, sizeof(int));
        state->stats->methods[i].trend.current.after = g_array_new(false, false, sizeof(int));
        state->stats->methods[i].trend.best.before = g_array_new(false, false, sizeof(int));
        state->stats->methods[i].trend.best.after = g_array_new(false, false, sizeof(int));
    }

    state->stats->starting_time = ms();

    long now;

    if (solver_conf->dont_solve) {
        generate_feasible_solution_if_needed(solver_conf, finder_conf, state);
        state->stats->ending_time = ms();
        goto QUIT;
    }

    // Main solver loop: call all the methods in a round robin way
    while (state->best_cost > 0) {
        if (timeout) {
            verbose("Time limit reached (%ds), stopping here", solver_conf->max_time);
            break;
        }

        if (!(state->cycle < cycles_limit)) {
            verbose("Cycles limit reached (%d), stopping here", cycles_limit);
            break;
        }

        // Generate a solution the first time (or each time if multistart=true)
        if (!generate_feasible_solution_if_needed(solver_conf, finder_conf, state))
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
            state->stats->best_restored_count++;
        }

        // Eventually print some stats
        if (get_verbosity()) {
            int lv = 2;
            if (get_verbosity() < lv) {
                now = clk();
                if (now - state->_last_log_time > 1000) {
                    // Print as verbose1 instead of verbose2 once each second
                    state->_last_log_time = now;
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
                       (double) 1000 * (double) state->cycle / (double) (ms() - state->stats->starting_time),
                       state->stats->move_count,
                       (double) 1000 * (double) state->stats->move_count /
                       (double) (ms() - state->stats->starting_time)
            );
        }

        // Real methods loop
        for (int i = 0; i < solver_conf->methods->len; i++) {
            heuristic_solver_method_callback_parameterized *method = &methods[i];
            verbose2("------------ %s BEGIN (%d) ------------",
                     method->name, state->current_cost);
            state->method = i;
            now = clk();
            if (collect_trend) {
                g_array_append_val(state->stats->methods[i].trend.current.before, state->current_cost);
                g_array_append_val(state->stats->methods[i].trend.best.before, state->best_cost);
            }
            method->method(state, method->param); // metaheuristic method call
            if (collect_trend) {
                g_array_append_val(state->stats->methods[i].trend.current.after, state->current_cost);
                g_array_append_val(state->stats->methods[i].trend.best.after, state->best_cost);
            }
            state->stats->methods[i].execution_time += (clk() - now);
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
        state->stats->cycle_count++;
    }

    state->stats->ending_time = ms();

    // Eventually print some stats after the resolution
    if (get_verbosity()) {
        verbose("=============== SOLVER FINISHED ===============\n"
                "Best = %d (found after %.2f seconds)\n"
                "Execution time = %.2f seconds\n"
                "Cycles = %d (cycles/s = %.2f)\n"
                "Moves = %d (moves/s = %.2f)\n"
                "Best restored %d times",
                state->best_cost,
                (double) (state->stats->best_solution_time - state->stats->starting_time) / 1000,
                (double) (state->stats->ending_time - state->stats->starting_time) / 1000,
                state->cycle,
                (double) 1000 * (double) state->cycle / (double) (state->stats->ending_time - state->stats->starting_time),
                state->stats->move_count,
                (double) 1000 * (double) state->stats->move_count / (double) (state->stats->ending_time - state->stats->starting_time),
                state->stats->best_restored_count
        );

        for (int i = 0; i < n_methods; i++) {
            verbose("%s:\n"
                    "   Execution time = %.2f seconds\n"
                    "   Moves = %d (moves/s = %.2f)\n"
                    "   Improvement count = %d (%d after first cycle\n"
                    "   Improvement delta = %d (%d after first cycle)",
                    state->methods_name[i],
                    (double) state->stats->methods[i].execution_time / 1000,
                    state->stats->methods[i].move_count,
                    (double) 1000 * (double) state->stats->methods[i].move_count /
                    (double) (state->stats->methods[i].execution_time),
                    state->stats->methods[i].improvement_count,
                    state->stats->methods[i].improvement_count_after_first_cycle,
                    state->stats->methods[i].improvement_delta,
                    state->stats->methods[i].improvement_delta_after_first_cycle);
        }

        if (collect_trend) {
            verbose("================== TREND ==================");
            /*                  LS                      SA
             * _0 | _7777 -> (-7000) -> 777* | 777 -> ( -400) -> 377*
             * _1 | __377 -> ( -200) -> 177* | 177 -> ( -400) -> 177
             */
            int trend_len = state->stats->methods[0].trend.current.before->len;
            int n_cycles_strlen = snprintf(NULL, 0, "%ld", state->cycle);
            int spacing = (int) n_cycles_strlen + 18;
            for (int m = 0; m < n_methods; m++) {
                const char *method_name = methods[m].short_name;
                verbosef("%*s", spacing - (int) strlen(method_name) / 2, method_name);
                spacing = 32 - (int) strlen(method_name);
            }
            verbosef("\n");
            for (int i = 0; i < trend_len; i++) {
                verbosef("%s%*d", i > 0 ? "\n" : "", n_cycles_strlen, i);
                for (int m = 0; m < n_methods; m++) {
                    int bef = ((int *) state->stats->methods[m].trend.current.before->data)[i];
                    int aft = ((int *) state->stats->methods[m].trend.current.after->data)[i];
                    int delta = aft - bef;
                    bool new_best =
                            ((int *) state->stats->methods[m].trend.best.after->data)[i] <
                            ((int *) state->stats->methods[m].trend.best.before->data)[i];
                    verbosef(" | %5d -> (%+6d) -> %5d%s", bef, delta, aft, new_best ? "*" : " ");
                }

            }
            verbosef("\n");
        }
    }

QUIT:
    free(state->methods_name);
    solution_destroy(state->current_solution);

    return strempty(solver->error);
}
/*
 * Must be called by metaherustics methods after each move:
 * eventually updates the best solution if the current is better
 * and update some stats->
 */
bool heuristic_solver_state_update(heuristic_solver_state *state) {
    bool improved = false;

    if (state->current_cost < state->best_cost) {
        verbose("%s: found new best solution of cost %d",
                state->methods_name[state->method], state->current_cost);
        assert(solution_satisfy_hard_constraints(state->current_solution));

        if (state->best_cost != INT_MAX) {
            int delta = state->current_cost - state->best_cost;
            state->stats->methods[state->method].improvement_count++;
            state->stats->methods[state->method].improvement_delta += delta;
            if (state->cycle > 0) {
                state->stats->methods[state->method].improvement_count_after_first_cycle++;
                state->stats->methods[state->method].improvement_delta_after_first_cycle += delta;
            }
        }

        state->stats->best_solution_time = ms();

        // Copy the current solution to the best
        state->best_cost = state->current_cost;
        solution_copy(state->best_solution, state->current_solution);
        improved = true;

        // Eventually callback
        if (state->config->new_best_callback.callback)
            state->config->new_best_callback.callback(
                    state->best_solution, state->stats,
                    state->config->new_best_callback.arg);
        
        assert(state->best_cost == solution_cost(state->best_solution));
    }

    state->stats->move_count++;
    state->stats->methods[state->method].move_count++;

   return improved;
}
