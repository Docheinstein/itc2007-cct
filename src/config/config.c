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
        "hc.idle = %d",
        cfg->solver_cycles_limit,
        cfg->solver_time_limit,
        BOOL_TO_STR(cfg->solver_multistart),
        solver_methods,
        cfg->hc_idle
    );

    free(solver_methods);

    return s;
}

void config_init(config *cfg) {
    cfg->solver_cycles_limit = -1;  // unlimited
    cfg->solver_time_limit = -1;    // unlimited
    cfg->solver_multistart = false;
    cfg->solver_methods = NULL;
    cfg->solver_n_methods = 0;
    cfg->hc_idle = 100000;
}

void config_destroy(config *cfg) {
    free(cfg->solver_methods);
}
