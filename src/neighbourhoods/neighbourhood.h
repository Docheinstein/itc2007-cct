#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

typedef enum neighbourhood_prediction_strategy {
    NEIGHBOURHOOD_PREDICT_ALWAYS,
    NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
    NEIGHBOURHOOD_PREDICT_NEVER
} neighbourhood_prediction_strategy;

typedef enum neighbourhood_performing_strategy {
    NEIGHBOURHOOD_PERFORM_ALWAYS,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE,
    NEIGHBOURHOOD_PERFORM_IF_BETTER,
    NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
    NEIGHBOURHOOD_PERFORM_NEVER,
} neighbourhood_performing_strategy;

#endif // NEIGHBOURHOOD_H