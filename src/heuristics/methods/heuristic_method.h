#ifndef HEURISTIC_METHOD_H
#define HEURISTIC_METHOD_H

typedef enum heuristic_method {
    HEURISTIC_METHOD_NONE,
    HEURISTIC_METHOD_LOCAL_SEARCH,
    HEURISTIC_METHOD_TABU_SEARCH,
    HEURISTIC_METHOD_HILL_CLIMBING,
    HEURISTIC_METHOD_SIMULATED_ANNEALING,
} heuristic_method;

const char * heuristic_method_to_string(heuristic_method method);
const char * heuristic_method_to_string_short(heuristic_method method);

#endif // HEURISTIC_METHOD_H