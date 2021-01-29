#include "heuristic_method.h"

const char *heuristic_method_to_string(heuristic_method method) {
    switch (method) {
    case HEURISTIC_METHOD_NONE:
        return "";
    case HEURISTIC_METHOD_LOCAL_SEARCH:
        return "Local Search";
    case HEURISTIC_METHOD_TABU_SEARCH:
        return "Tabu Search";
    case HEURISTIC_METHOD_HILL_CLIMBING:
        return "Hill Climbing";
    case HEURISTIC_METHOD_SIMULATED_ANNEALING:
        return "Simulated Annealing";
    default:
        return "?";
    }
}

const char *heuristic_method_to_string_short(heuristic_method method) {
    switch (method) {
    case HEURISTIC_METHOD_NONE:
        return "";
    case HEURISTIC_METHOD_LOCAL_SEARCH:
        return "ls";
    case HEURISTIC_METHOD_TABU_SEARCH:
        return "ts";
    case HEURISTIC_METHOD_HILL_CLIMBING:
        return "hc";
    case HEURISTIC_METHOD_SIMULATED_ANNEALING:
        return "sa";
    default:
        return "?";
    }
}
