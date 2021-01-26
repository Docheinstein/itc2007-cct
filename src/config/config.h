#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <resolution_method.h>

typedef struct config {
    // Solver
    struct {
        resolution_method *methods;
        int n_methods;
        int cycles_limit;
        int time_limit;
        bool multistart;
        int restore_best_after_cycles;
    } solver;

    // Hill Climbing
    struct {
        int idle;
    } hc;

    // Tabu Search
    struct {
        int idle;
        int tenure;
        double frequency_penalty_coeff;
        bool random_pick;
        bool steepest;
        bool clear_on_best;
    } ts;

    // Simulated Annealing
    struct {
        int idle;
        double temperature;
        double min_temperature;
        double temperature_length_coeff;
        double cooling_rate;
    } sa;
} config;

void config_default(config *cfg);
void config_destroy(config *cfg);

char * config_to_string(const config *cfg);



#endif // CONFIG_H