#ifndef HEURISTIC_SOLVER_H
#define HEURISTIC_SOLVER_H

#include "solution.h"

typedef struct heuristic_solver_state {
    solution *current_solution;
    solution *best_solution;
    int current_cost;
    int best_cost;
    int cycle;
} heuristic_solver_state;

bool heuristic_solver_state_update(heuristic_solver_state *state);

typedef void (*heuristic_method)(
        heuristic_solver_state *    /* current resolution state */,
        void *                      /* any arg */);

typedef struct heuristic_method_parameterized {
    heuristic_method method;
    void *param;
    const char *name;
} heuristic_method_parameterized;

typedef struct heuristic_solver_config {
    solution *starting_solution;
    int time_limit;
    int cycles_limit;
    bool multistart;
    GArray *methods;
} heuristic_solver_config;


typedef struct heuristic_solver {
    char *error;
    heuristic_solver_state state;
} heuristic_solver;

void heuristic_solver_config_init(heuristic_solver_config *config);
void heuristic_solver_config_add_method(heuristic_solver_config *config,
                                        heuristic_method method,
                                        void *param, const char *name);
void heuristic_solver_config_destroy(heuristic_solver_config *config);

void heuristic_solver_init(heuristic_solver *solver);
void heuristic_solver_destroy(heuristic_solver *solver);

const char * heuristic_solver_get_error(heuristic_solver *solver);

bool heuristic_solver_solve(heuristic_solver *solver, const heuristic_solver_config *config,
                            solution *solution);

#endif // HEURISTIC_SOLVER_H