#include "resolution_method.h"

const char * resolution_method_to_string(resolution_method method) {
    switch (method) {
    case RESOLUTION_METHOD_EXACT:
        return "exact";
    case RESOLUTION_METHOD_LOCAL_SEARCH:
        return "local search";
    case RESOLUTION_METHOD_TABU_SEARCH:
        return "tabu search";
    case RESOLUTION_METHOD_HILL_CLIMBING:
        return "hill climbing";
    default:
        return "unknown";
    }
}