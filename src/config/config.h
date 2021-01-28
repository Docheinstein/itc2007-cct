#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <heuristics/methods/heuristic_method.h>

typedef struct config {
    // Solver
    struct {
        struct {
            heuristic_method *data;
            int len;
        } methods;
        int max_time;
        int max_cycles;
        bool multistart;
        int restore_best_after_cycles;
    } solver;

    // Feasible solution finder
    struct {
        double ranking_randomness;
    } finder;

    // Local Search
    struct {
        bool steepest;
    } ls;

    // Hill Climbing
    struct {
        int max_idle;
    } hc;

    // Tabu Search
    struct {
        int max_idle;
        int tabu_tenure;
        double frequency_penalty_coeff;
        bool random_pick;
        bool steepest;
        bool clear_on_best;
    } ts;

    // Simulated Annealing
    struct {
        int max_idle;
        double initial_temperature;
        double cooling_rate;
        double min_temperature;
        double temperature_length_coeff;
    } sa;
} config;

void config_init(config *cfg);
void config_destroy(config *cfg);

char * config_to_string(const config *cfg);

#endif // CONFIG_H