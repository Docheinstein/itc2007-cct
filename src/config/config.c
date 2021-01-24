#include <stdlib.h>
#include <log/debug.h>
#include <utils/mem_utils.h>
#include <string.h>
#include "config.h"
#include "utils/str_utils.h"

char *config_to_string(const config *cfg) {
    char *methods[cfg->n_methods];
    for (int i = 0; i < cfg->n_methods; i++)
        methods[i] = strdup(resolution_method_to_string(cfg->methods[i]));
    char *solver_methods = strjoin(methods, cfg->n_methods, ", ");
    for (int i = 0; i < cfg->n_methods; i++)
        free(methods[i]);

    char *s = strmake(
        "solver.methods = %s\n"
        "hc.idle = %d",
        solver_methods,
        cfg->hc_idle
    );

    free(solver_methods);

    return s;
}

void config_init(config *cfg) {
    cfg->methods = NULL;
    cfg->n_methods = 0;
    cfg->hc_idle = 100000;
}

void config_destroy(config *cfg) {
    free(cfg->methods);
}
