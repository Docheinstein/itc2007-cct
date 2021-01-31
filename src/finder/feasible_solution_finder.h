#ifndef FEASIBLE_SOLUTION_FINDER_H
#define FEASIBLE_SOLUTION_FINDER_H

#include <stdbool.h>
#include "solution/solution.h"

/*
 * Finder of an initial feasible solution.
 * The logic to find a feasible solution is the following:
 * For each course an "assignment difficulty" is computed,
 * depending on how many (hard) constraints the course is
 * associated with.
 * The lectures of the courses are then sorted by descending
 * difficulty and assigned to the first available (room, day, slot)
 * that does not break any hard constraint.
 * If such a (room, day, slot) does not exists,
 * the finder fails to provide a feasible solution.
 *
 * In order to generate different solutions (which is
 * necessary to implement e.g. multistart), a `ranking_randomness`
 * parameter is used, which eventually alters the "assignment
 * difficulty" of a course's lectures.
 * Higher value of `ranking_randomness` makes the generated
 * solutions "more random", but makes it harder to find feasible ones.
 * If `ranking_randomness` is 0, the same solution is generated
 * each time.
 */
typedef struct feasible_solution_finder_config {
    double ranking_randomness;
} feasible_solution_finder_config;

typedef struct feasible_solution_finder {
    char *error;
} feasible_solution_finder;

void feasible_solution_finder_config_default(feasible_solution_finder_config *config);

void feasible_solution_finder_init(feasible_solution_finder *finder);
void feasible_solution_finder_destroy(feasible_solution_finder *finder);

bool feasible_solution_finder_try_find(
        feasible_solution_finder *finder,
        const feasible_solution_finder_config *config,
        solution *sol);

bool feasible_solution_finder_find(
        feasible_solution_finder *finder,
        const feasible_solution_finder_config *config,
        solution *solution);

const char * feasible_solution_finder_find_get_error(feasible_solution_finder *finder);

#endif // FEASIBLE_SOLUTION_FINDER_H