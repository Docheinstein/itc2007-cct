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
        "finder.ranking_randomness = %.2f\n"
        "hc.max_idle = %ld\n"
        "ts.max_idle = %ld\n"
        "ts.tabu_tenure = %d\n"
        "ts.frequency_penalty_coeff = %.2f\n"
        "sa.max_idle = %ld\n"
        "sa.initial_temperature = %.2f\n"
        "sa.cooling_rate = %.2f\n"
        "sa.min_temperature = %.2f\n"
        "sa.temperature_length_coeff = %.2f",
        solver_methods,
        cfg->solver.max_time,
        cfg->solver.max_cycles,
        booltostr(cfg->solver.multistart),
        cfg->solver.restore_best_after_cycles,
        // ---
        cfg->finder.ranking_randomness,
        // ---
        cfg->hc.max_idle,
        // ---
        cfg->ts.max_idle,
        cfg->ts.tabu_tenure,
        cfg->ts.frequency_penalty_coeff,
        // ---
        cfg->sa.max_idle,
        cfg->sa.initial_temperature,
        cfg->sa.cooling_rate,
        cfg->sa.min_temperature,
        cfg->sa.temperature_length_coeff
    );

    free(solver_methods);

    return s;
}

void config_init(config *cfg) {
    cfg->solver.methods = g_array_new(false, false, sizeof(heuristic_method));
    cfg->solver.max_time = 60;
    cfg->solver.max_cycles = -1;
    cfg->solver.multistart = false;
    cfg->solver.restore_best_after_cycles = 20;

    // Default methods: HC+SA
//    heuristic_method *hc = mallocx(1, sizeof(heuristic_method));
//    heuristic_method *sa = mallocx(1, sizeof(heuristic_method));
//    *hc = HEURISTIC_METHOD_HILL_CLIMBING;
//    *sa = HEURISTIC_METHOD_SIMULATED_ANNEALING;
    heuristic_method hc = HEURISTIC_METHOD_HILL_CLIMBING;
    heuristic_method sa = HEURISTIC_METHOD_SIMULATED_ANNEALING;

    g_array_append_val(cfg->solver.methods, hc);
    g_array_append_val(cfg->solver.methods, sa);

    feasible_solution_finder_config_default(&cfg->finder);
    local_search_params_default(&cfg->ls);
    hill_climbing_params_default(&cfg->hc);
    tabu_search_params_default(&cfg->ts);
    simulated_annealing_params_default(&cfg->sa);
}

void config_destroy(config *cfg) {
    g_array_free(cfg->solver.methods, true);
}