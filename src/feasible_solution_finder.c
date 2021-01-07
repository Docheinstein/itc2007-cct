#include "feasible_solution_finder.h"
#include "str_utils.h"
#include <stdlib.h>

void feasible_solution_finder_init(feasible_solution_finder *finder) {
    finder->error = NULL;
}

void feasible_solution_finder_destroy(feasible_solution_finder *finder) {
    free(finder->error);
}

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   const model *model, solution *solution) {
    finder->error = strmake("don't know what to do");

    return false;
}

const char *feasible_solution_finder_find_get_error(feasible_solution_finder *finder) {
    return finder->error;
}
