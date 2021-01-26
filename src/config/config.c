#include <stdlib.h>
#include <log/debug.h>
#include <utils/mem_utils.h>
#include <string.h>
#include <utils/def_utils.h>
#include "config.h"
#include "utils/str_utils.h"

char *config_to_string(const config *cfg) {
    char *methods[cfg->solver.n_methods];
    for (int i = 0; i < cfg->solver.n_methods; i++)
        methods[i] = strdup(resolution_method_to_string(cfg->solver.methods[i]));
    char *solver_methods = strjoin(methods, cfg->solver.n_methods, ", ");
    for (int i = 0; i < cfg->solver.n_methods; i++)
        free(methods[i]);

    char *s = strmake(
        "solver.methods = %s\n"
        "solver.cycles = %d\n"
        "solver.time = %d\n"
        "solver.multistart = %s\n"
        "solver.cycles_limit = %d\n"
        "hc.idle = %d\n"
        "ts.idle = %d\n"
        "ts.tenure = %d\n"
        "ts.frequency_penalty_coeff = %g\n"
        "ts.random_pick = %s\n"
        "ts.steepest = %s\n"
        "ts.clear_on_best = %s",
        solver_methods,
        cfg->solver.cycles_limit,
        cfg->solver.time_limit,
        BOOL_TO_STR(cfg->solver.multistart),
        cfg->solver.restore_best_after_cycles,
        cfg->hc.idle,
        cfg->ts.idle,
        cfg->ts.tenure,
        cfg->ts.frequency_penalty_coeff,
        BOOL_TO_STR(cfg->ts.random_pick),
        BOOL_TO_STR(cfg->ts.steepest),
        BOOL_TO_STR(cfg->ts.clear_on_best)
    );

    free(solver_methods);

    return s;
}

void config_default(config *cfg) {
    cfg->solver.methods = NULL;
    cfg->solver.n_methods = 0;
    cfg->solver.cycles_limit = -1;  // unlimited
    cfg->solver.time_limit = -1;    // unlimited
    cfg->solver.multistart = false;
    cfg->solver.restore_best_after_cycles = -1; // never

    cfg->hc.idle = 100000;

    cfg->ts.idle = 10000;
    cfg->ts.tenure = 14;
    cfg->ts.frequency_penalty_coeff = 1.2;
    cfg->ts.random_pick = false;
    cfg->ts.steepest = false;
    cfg->ts.clear_on_best = false;

    cfg->sa.idle = 100000;
    cfg->sa.temperature = 1.8;
    cfg->sa.min_temperature = 0.08;
    cfg->sa.temperature_length_coeff = 1;
    cfg->sa.cooling_rate = 0.96;

}

void config_destroy(config *cfg) {
    free(cfg->solver.methods);
}
