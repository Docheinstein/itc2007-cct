#ifndef NEIGHBOURHOOD_STABILIZE_ROOM_H
#define NEIGHBOURHOOD_STABILIZE_ROOM_H

#include "solution.h"
#include "neighbourhood.h"

typedef struct neighbourhood_stabilize_room_move {
    int c1, r2;
} neighbourhood_stabilize_room_move;


typedef struct neighbourhood_stabilize_room_iter {
    solution *solution;
    bool end;
//    int cr_index;
    neighbourhood_stabilize_room_move current;
} neighbourhood_stabilize_room_iter;


typedef struct neighbourhood_stabilize_room_result {
    solution_fingerprint_t fingerprint_plus;
    solution_fingerprint_t fingerprint_minus;
    int delta_cost;
    // TODO: remove, keep just for debug
    int delta_cost_room_capacity;
    int delta_cost_min_working_days;
    int delta_cost_curriculum_compactness;
    int delta_cost_room_stability;
} neighbourhood_stabilize_room_result;

void neighbourhood_stabilize_room_move_copy(
        neighbourhood_stabilize_room_move *dest,
        const neighbourhood_stabilize_room_move *src);

void neighbourhood_stabilize_room_iter_init(
        neighbourhood_stabilize_room_iter *iter, solution *sol);
void neighbourhood_stabilize_room_iter_destroy(
        neighbourhood_stabilize_room_iter *iter);
bool neighbourhood_stabilize_room_iter_next(
        neighbourhood_stabilize_room_iter *iter, neighbourhood_stabilize_room_move *move);
bool neighbourhood_stabilize_room(
        solution *sol,
        neighbourhood_stabilize_room_move *move,
        neighbourhood_prediction_strategy predict_cost,
        neighbourhood_prediction_strategy predict_fingerprint,
        neighbourhood_performing_strategy perform,
        neighbourhood_stabilize_room_result *result);


#endif // NEIGHBOURHOOD_STABILIZE_ROOM_H