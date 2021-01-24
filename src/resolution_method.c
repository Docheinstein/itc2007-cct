#include "resolution_method.h"

const char * resolution_method_to_string(resolution_method method) {
    switch (method) {
    case RESOLUTION_METHOD_NONE:
        return "";
    case RESOLUTION_METHOD_EXACT:
        return "Exact";
    case RESOLUTION_METHOD_LOCAL_SEARCH:
        return "Local Search";
    case RESOLUTION_METHOD_TABU_SEARCH:
        return "Tabu Search";
    case RESOLUTION_METHOD_HILL_CLIMBING:
        return "Hill Climbing";
    case RESOLUTION_METHOD_SIMULATED_ANNEALING:
        return "Simulated Annealing";
    default:
        return "?";
    }
}