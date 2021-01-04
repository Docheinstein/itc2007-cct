#ifndef EXACT_SOLVER_H
#define EXACT_SOLVER_H

#include "model.h"
#include "solution.h"

typedef struct exact_solver_config {
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

void exact_solver_config_init(exact_solver_config *config,
                              double s_room_capacity, double s_minimum_working_days,
                              double s_curriculum_compactness, double s_room_stability);
bool exact_solver_solve(exact_solver_config *config, model *model, solution *solution);

#endif // EXACT_SOLVER_H