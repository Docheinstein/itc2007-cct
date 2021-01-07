#include "resolution_method.h"

const char * resolution_method_to_string(resolution_method method) {
    switch (method) {
    case RESOLUTION_METHOD_EXACT:
        return "exact";
    case RESOLUTION_METHOD_TABU:
        return "tabu";
    default:
        return "unknown";
    }
}