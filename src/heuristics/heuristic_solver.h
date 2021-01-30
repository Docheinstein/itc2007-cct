#ifndef HEURISTIC_SOLVER_H
#define HEURISTIC_SOLVER_H

#include <finder/feasible_solution_finder.h>
#include "solution/solution.h"

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

bool heuristic_solver_state_update(heuristic_solver_state *state);

typedef void (*heuristic_solver_method_callback)(
        heuristic_solver_state *    /* current resolution state */,
        void *                      /* any arg */);

typedef struct heuristic_solver_method_callback_parameterized {
    heuristic_solver_method_callback method;
    void *param;
    const char *name;
} heuristic_solver_method_callback_parameterized;

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

const char * heuristic_solver_get_error(heuristic_solver *solver);

bool heuristic_solver_solve(heuristic_solver *solver,
                            const heuristic_solver_config *solver_conf,
                            const feasible_solution_finder_config *finder_conf,
                            solution *solution);

#endif // HEURISTIC_SOLVER_H