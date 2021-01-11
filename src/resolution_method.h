#ifndef RESOLUTION_METHOD_H
#define RESOLUTION_METHOD_H


typedef enum resolution_method {
    RESOLUTION_METHOD_EXACT,
    RESOLUTION_METHOD_LOCAL_SEARCH,
    RESOLUTION_METHOD_TABU_SEARCH,
    RESOLUTION_METHOD_DEFAULT = RESOLUTION_METHOD_LOCAL_SEARCH
} resolution_method;

const char * resolution_method_to_string(resolution_method method);

#endif // RESOLUTION_METHOD_H