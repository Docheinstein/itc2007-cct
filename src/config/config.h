#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <resolution_method.h>

typedef struct config {
    // Solver
    int solver_cycles_limit;
    int solver_time_limit;
    bool solver_multistart;
    resolution_method *solver_methods;
    int solver_n_methods;

    // Hill Climbing
    int hc_idle;
} config;

void config_init(config *cfg);
void config_destroy(config *cfg);

char * config_to_string(const config *cfg);



#endif // CONFIG_H