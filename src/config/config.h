#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <glib.h>
#include <heuristics/methods/deep_local_search.h>
#include "heuristics/methods/heuristic_method.h"
#include "heuristics/heuristic_solver.h"
#include "heuristics/methods/local_search.h"
#include "heuristics/methods/hill_climbing.h"
#include "heuristics/methods/tabu_search.h"
#include "heuristics/methods/simulated_annealing.h"

/* Options: either of a config file or given at the command line with -o */

typedef struct config {
    struct solver {
        GArray *methods;
        int max_time;
        int max_cycles;
        bool multistart;
        int restore_best_after_cycles;
    } solver;
    feasible_solution_finder_config finder;
    deep_local_search_params dls;
    local_search_params ls;
    hill_climbing_params hc;
    tabu_search_params ts;
    simulated_annealing_params sa;
} config;

void config_init(config *cfg);
void config_destroy(config *cfg);

char * config_to_string(const config *cfg);

#endif // CONFIG_H