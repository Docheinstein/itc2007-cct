#ifndef EXACT_SOLVER_H
#define EXACT_SOLVER_H

#include "model.h"
#include "solution.h"

typedef struct exact_solver_config {
    // Gurobi
    bool grb_verbose;
    double grb_time_limit;
    char * grb_write_lp;

    // Hard constraints (enable/disable)
    bool h_lectures;
    bool h_room_occupancy;
    bool h_conflicts;
    bool h_availability;

    // Soft constraints (cost)
    double s_room_capacity;
    double s_minimum_working_days;
    double s_curriculum_compactness;
    double s_room_stability;
} exact_solver_config;

typedef struct exact_solver {
    double objective;
    const char *error;
} exact_solver;


void exact_solver_config_init(exact_solver_config *config);
void exact_solver_config_destroy(exact_solver_config *config);

void exact_solver_init(exact_solver *solver);
void exact_solver_destroy(exact_solver *solver);

bool exact_solver_solve(exact_solver *solver, exact_solver_config *config, solution *solution);

const char *exact_solver_get_error(exact_solver *solver);

#endif // EXACT_SOLVER_H