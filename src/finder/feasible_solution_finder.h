#ifndef FEASIBLE_SOLUTION_FINDER_H
#define FEASIBLE_SOLUTION_FINDER_H

#include <stdbool.h>
#include "model/model.h"
#include "solution/solution.h"

typedef struct feasible_solution_finder_config {
    double ranking_randomness;
} feasible_solution_finder_config;

typedef struct feasible_solution_finder {
    char *error;
} feasible_solution_finder;

void feasible_solution_finder_config_init(feasible_solution_finder_config *config);
void feasible_solution_finder_config_destroy(feasible_solution_finder_config *config);

void feasible_solution_finder_init(feasible_solution_finder *finder);
void feasible_solution_finder_destroy(feasible_solution_finder *finder);

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   const feasible_solution_finder_config *config,
                                   solution *solution);

const char * feasible_solution_finder_find_get_error(feasible_solution_finder *finder);

#endif // FEASIBLE_SOLUTION_FINDER_H