#include <stdlib.h>
#include <log/debug.h>
#include <utils/mem_utils.h>
#include <string.h>
#include <utils/def_utils.h>
#include "config.h"
#include "utils/str_utils.h"

char *config_to_string(const config *cfg) {
    char *methods[cfg->solver_n_methods];
    for (int i = 0; i < cfg->solver_n_methods; i++)
        methods[i] = strdup(resolution_method_to_string(cfg->solver_methods[i]));
    char *solver_methods = strjoin(methods, cfg->solver_n_methods, ", ");
    for (int i = 0; i < cfg->solver_n_methods; i++)
        free(methods[i]);

    char *s = strmake(
        "solver.cycles = %d\n"
        "solver.time = %d\n"
        "solver.multistart = %s\n"
        "solver.methods = %s\n"
        "hc.idle = %d\n"
        "ts.idle = %d\n"
        "ts.tenure = %d\n"
        "ts.frequency_penalty_coeff = %g\n"
        "ts.random_pick = %s\n"
        "ts.steepest = %s\n"
        "ts.clear_on_new_best = %s",
        cfg->solver_cycles_limit,
        cfg->solver_time_limit,
        BOOL_TO_STR(cfg->solver_multistart),
        solver_methods,
        cfg->hc_idle,
        cfg->ts_idle,
        cfg->ts_tenure,
        cfg->ts_frequency_penalty_coeff,
        BOOL_TO_STR(cfg->ts_random_pick),
        BOOL_TO_STR(cfg->ts_steepest),
        BOOL_TO_STR(cfg->ts_clear_on_new_best)
    );

    free(solver_methods);

    return s;
}

void config_default(config *cfg) {
    cfg->solver_cycles_limit = -1;  // unlimited
    cfg->solver_time_limit = -1;    // unlimited
    cfg->solver_multistart = false;
    cfg->solver_methods = NULL;
    cfg->solver_n_methods = 0;
    cfg->hc_idle = 100000;
    cfg->ts_idle = 10000;
    cfg->ts_tenure = 14;
    cfg->ts_frequency_penalty_coeff = 1.2;
    cfg->ts_random_pick = false;
    cfg->ts_steepest = false;
    cfg->ts_clear_on_new_best = false;
}

void config_destroy(config *cfg) {
    free(cfg->solver_methods);
}
