#ifndef HEURISTIC_SOLVER_H
#define HEURISTIC_SOLVER_H

#include <finder/feasible_solution_finder.h>
#include "solution/solution.h"

/*
 * Metaheuristics Solver.
 * This solver is really general purpose and does not implement
 * any metaheuristics actually, instead it can be configured
 * to use any metaheuristics by adding those to the config via
 * `heuristic_solver_config_add_method`.
 * This solver is responsible for call the methods in
 * round robin order, collects some stats and quit the resolution
 * when an exit condition occurs (max_time or max_cycles).
 */

typedef struct heuristic_solver_state_method_stats {
    int improvement;
    long execution_time;
    long move_count;
} heuristic_solver_state_method_stats;

typedef struct heuristic_solver_state {
    const model *model;

    solution *current_solution;
    solution *best_solution;
    int current_cost;
    int best_cost;

    long cycle;
    int method;

    long non_improving_best_cycles;
    long non_improving_current_cycles;

    bool collect_stats;
    bool collect_trend;

    const char **methods_name;

    // Stats: not strictly necessary for the resolution
    struct {
        long move_count;

        heuristic_solver_state_method_stats *methods;

        long starting_time;
        long best_solution_time;
        long ending_time;

        long last_log_time;

        GArray *trend_current;
        GArray *trend_best;
    } stats;

} heuristic_solver_state;

/*
 * Callback implemented by metaheuristics methods
 * for being called by the solver.
 */
typedef void (*heuristic_solver_method_callback)(
        heuristic_solver_state * /* current resolution state */,
        void * /* any arg, provided to `heuristic_solver_config_add_method` */);

typedef struct heuristic_solver_method_callback_parameterized {
    heuristic_solver_method_callback method;
    void *param;
    const char *name;
} heuristic_solver_method_callback_parameterized;

/*
 * Config of the solver.
 * `max_time`: quit the solver after `max_time` seconds.
 * `max_cycles`: quit the solver after `max_cycles` cycles (
 *      a cycle is a single execution of all the methods)
 * `multistart`: generate a new initial solution after each cycle
 *      (useful with methods hanging at local minimum. e.g. local search)
 * `restore_best_after_cycles`: restore the best known solution after
 *      `restore_best_after_cycles` cycles of non improving cost (relative to the best)
 *  `starting_solution`: start from this solution instead of generating one.
 *       if `multistart` is true, starts always from this starting solution
 *  `dont_solve`: just generate the initial feasible solution and quit
 */
typedef struct heuristic_solver_config {
    GArray *methods;
    int max_time;
    int max_cycles;
    bool multistart;
    int restore_best_after_cycles;

    solution *starting_solution;
    bool dont_solve;
} heuristic_solver_config;

typedef struct heuristic_solver {
    char *error;
    heuristic_solver_state state;
} heuristic_solver;

void heuristic_solver_config_init(heuristic_solver_config *config);
void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_solver_method_callback method,
                                        void *param, const char *name);
void heuristic_solver_config_destroy(heuristic_solver_config *config);

void heuristic_solver_init(heuristic_solver *solver);
void heuristic_solver_destroy(heuristic_solver *solver);

bool heuristic_solver_solve(heuristic_solver *solver,
                            const heuristic_solver_config *solver_conf,
                            const feasible_solution_finder_config *finder_conf,
                            solution *solution);

/* Must be called by `heuristic_solver_method_callback` when a move is performed */
bool heuristic_solver_state_update(heuristic_solver_state *state);

const char * heuristic_solver_get_error(heuristic_solver *solver);

#endif // HEURISTIC_SOLVER_H