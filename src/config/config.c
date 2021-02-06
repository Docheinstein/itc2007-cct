#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "utils/str_utils.h"

char *config_to_string(const config *cfg) {
    char *methods_str[cfg->solver.methods->len];
    heuristic_method *methods = (heuristic_method *) cfg->solver.methods->data;
    for (int i = 0; i < cfg->solver.methods->len; i++)
        methods_str[i] = strdup(heuristic_method_to_string(methods[i]));
    char *solver_methods = strjoin(methods_str, cfg->solver.methods->len, ", ");
    for (int i = 0; i < cfg->solver.methods->len; i++)
        free(methods_str[i]);

    char *s = strmake(
        "solver.methods = %s\n"
        "solver.max_time = %d\n"
        "solver.max_cycles = %d\n"
        "solver.multistart = %s\n"
        "solver.restore_best_after_cycles = %d\n"
        "finder.ranking_randomness = %.4f\n"
        "ls.max_distance_from_best_ratio = %.4f\n"
        "hc.max_idle = %ld\n"
        "hc.max_idle_near_best_coeff = %.4f\n"
        "hc.near_best_ratio = %.4f\n"
        "ts.max_idle = %ld\n"
        "ts.max_idle_near_best_coeff = %.4f\n"
        "ts.near_best_ratio = %.4f\n"
        "ts.tabu_tenure = %d\n"
        "ts.frequency_penalty_coeff = %.4f\n"
        "sa.initial_temperature = %.5f\n"
        "sa.cooling_rate = %.5f\n"
        "sa.temperature_length_coeff = %.5f\n"
        "sa.min_temperature = %.5f\n"
        "sa.min_temperature_near_best_coeff = %.5f\n"
        "sa.near_best_ratio = %.5f\n"
        "sa.reheat_coeff = %.5f",
        solver_methods,
        cfg->solver.max_time,
        cfg->solver.max_cycles,
        booltostr(cfg->solver.multistart),
        cfg->solver.restore_best_after_cycles,
        // ---
        cfg->finder.ranking_randomness,
        // ---
        cfg->ls.max_distance_from_best_ratio,
        // ---
        cfg->hc.max_idle,
        cfg->hc.max_idle_near_best_coeff,
        cfg->hc.near_best_ratio,
        // ---
        cfg->ts.max_idle,
        cfg->ts.max_idle_near_best_coeff,
        cfg->ts.near_best_ratio,
        cfg->ts.tabu_tenure,
        cfg->ts.frequency_penalty_coeff,
        // ---
        cfg->sa.initial_temperature,
        cfg->sa.cooling_rate,
        cfg->sa.temperature_length_coeff,
        cfg->sa.min_temperature,
        cfg->sa.min_temperature_near_best_coeff,
        cfg->sa.near_best_ratio,
        cfg->sa.reheat_coeff
    );

    free(solver_methods);

    return s;
}

void config_init(config *cfg) {
    cfg->solver.methods = g_array_new(false, false, sizeof(heuristic_method));
    cfg->solver.max_time = 60;
    cfg->solver.max_cycles = -1;
    cfg->solver.multistart = false;
    cfg->solver.restore_best_after_cycles = 50;

    feasible_solution_finder_config_default(&cfg->finder);
    local_search_params_default(&cfg->ls);
    hill_climbing_params_default(&cfg->hc);
    tabu_search_params_default(&cfg->ts);
    simulated_annealing_params_default(&cfg->sa);
}

void config_destroy(config *cfg) {
    g_array_free(cfg->solver.methods, true);
}