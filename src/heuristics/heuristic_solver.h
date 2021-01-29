#ifndef HEURISTIC_SOLVER_H
#define HEURISTIC_SOLVER_H

#include <finder/feasible_solution_finder.h>
#include "solution/solution.h"

typedef struct heuristic_solver_state {
    const model *model;

    solution *current_solution;
    solution *best_solution;
    int current_cost;
    int best_cost;

    int cycle;

    long starting_time;
    long best_solution_time;
    long ending_time;
} heuristic_solver_state;

bool heuristic_solver_state_update(heuristic_solver_state *state);

typedef void (*heuristic_solver_method_callback)(
        heuristic_solver_state *    /* current resolution state */,
        void *                      /* any arg */);

typedef struct heuristic_solver_method_callback_param {
    heuristic_solver_method_callback method;
    void *param;
    const char *name;
} heuristic_solver_method_callback_param;

typedef struct heuristic_solver_config {
    solution *starting_solution;
    GArray *methods;
    int max_time;
    int max_cycles;
    bool multistart;
    int restore_best_after_cycles;
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