#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

typedef enum neighbourhood_predict_feasibility_strategy {
    NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
    NEIGHBOURHOOD_PREDICT_FEASIBILITY_NEVER,
} neighbourhood_predict_feasibility_strategy;

typedef enum neighbourhood_predict_cost_strategy {
    NEIGHBOURHOOD_PREDICT_COST_ALWAYS,
    NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE,
    NEIGHBOURHOOD_PREDICT_COST_NEVER
} neighbourhood_predict_cost_strategy;

typedef enum neighbourhood_perform_strategy {
    NEIGHBOURHOOD_PERFORM_ALWAYS,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE,
    NEIGHBOURHOOD_PERFORM_IF_BETTER,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
    NEIGHBOURHOOD_PERFORM_NEVER,
} neighbourhood_perform_strategy;

#endif // NEIGHBOURHOOD_H