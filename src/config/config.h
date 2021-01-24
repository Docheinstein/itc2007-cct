#ifndef CONFIG_H
#define CONFIG_H

#include <resolution_method.h>

typedef struct config {
    resolution_method *methods;
    int n_methods;
    int hc_idle;
} config;

void config_init(config *cfg);
void config_destroy(config *cfg);

char * config_to_string(const config *cfg);



#endif // CONFIG_H