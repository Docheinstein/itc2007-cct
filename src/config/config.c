#include <stdlib.h>
#include <utils/mem_utils.h>
#include <string.h>
#include "config.h"
#include "utils/str_utils.h"
#include "config_parser.h"

char *config_to_string(const config *cfg) {
    char *methods[cfg->solver.methods.len];
    for (int i = 0; i < cfg->solver.methods.len; i++)
        methods[i] = strdup(heuristic_method_to_string(cfg->solver.methods.data[i]));
    char *solver_methods = strjoin(methods, cfg->solver.methods.len, ", ");
    for (int i = 0; i < cfg->solver.methods.len; i++)
        free(methods[i]);

    char *s = strmake(
        "solver.methods = %s\n"
        "solver.max_time = %d\n"
        "solver.max_cycles = %d\n"
        "solver.multistart = %s\n"
        "solver.restore_best_after_cycles = %d\n"
        "\n"
        "finder.ranking_randomness = %g\n"
        "\n"
        "ls.steepest = %s\n"
        "\n"
        "hc.max_idle = %d\n"
        "\n"
        "ts.max_idle = %d\n"
        "ts.tabu_tenure = %d\n"
        "ts.frequency_penalty_coeff = %g\n"
        "ts.random_pick = %s\n"
        "ts.steepest = %s\n"
        "ts.clear_on_best = %s\n"
        "\n"
        "sa.max_idle = %d\n"
        "sa.initial_temperature = %g\n"
        "sa.cooling_rate = %d\n"
        "sa.min_temperature = %d\n"
        "sa.temperature_length_coeff = %d",
        solver_methods,
        cfg->solver.max_time,
        cfg->solver.max_cycles,
        booltostr(cfg->solver.multistart),
        cfg->solver.restore_best_after_cycles,
        // ---
        cfg->finder.ranking_randomness,
        // ---
        booltostr(cfg->ls.steepest),
        // ---
        cfg->hc.max_idle,
        // ---
        cfg->ts.max_idle,
        cfg->ts.tabu_tenure,
        cfg->ts.frequency_penalty_coeff,
        booltostr(cfg->ts.random_pick),
        booltostr(cfg->ts.steepest),
        booltostr(cfg->ts.clear_on_best),
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
    cfg->solver.methods.data = NULL;
    cfg->solver.methods.len = 0;
    cfg->solver.max_time = -1;
    cfg->solver.max_cycles = -1;
    cfg->solver.multistart = false;
    cfg->solver.restore_best_after_cycles = -1;

    cfg->finder.ranking_randomness = 0.66;

    cfg->ls.steepest = true;

    cfg->hc.max_idle = 120000;

    cfg->ts.max_idle = 400;
    cfg->ts.tabu_tenure = 120;
    cfg->ts.frequency_penalty_coeff = 1.2;
    cfg->ts.random_pick = true;
    cfg->ts.steepest = true;
    cfg->ts.clear_on_best = true;

    cfg->sa.max_idle = 80000;
    cfg->sa.initial_temperature = 1.5;
    cfg->sa.cooling_rate = 0.96;
    cfg->sa.min_temperature = 0.08;
    cfg->sa.temperature_length_coeff = 1;
}

void config_destroy(config *cfg) {
    free(cfg->solver.methods.data);
}
