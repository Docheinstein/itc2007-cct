#ifndef SWAP_H
#define SWAP_H

#include "solution/solution.h"
#include "neighbourhood.h"

/*
 * The only neighbourhood implement right now.
 * A `swap_move` is defined by the tuple (lecture_src, room_dest, day_dest, slot_dest),
 * and therefore defines a new assignment for a certain lecture.
 * If the target (room, day, slot) is already used by another lecture,
 * the two are swapped.
 */

typedef struct swap_move {
    /* Attributes that identify a swap. */
    int l1;          // lecture (from)
    int r2, d2, s2;  // assignment (to)
    struct {
        /* The helper's attributes depend on the current state
         * of the solution, and are not necessary to identify
         * a swap, but might be useful to keep in memory in order
         * to avoid to recompute them often. */
        int c1, r1, d1, s1; // from
        int l2, c2;         // to
    } helper;
} swap_move;

typedef struct swap_iter {
    const solution *solution;
    swap_move move;
    bool end;
    int i;
} swap_iter;

typedef struct swap_result {
    bool feasible;
    struct {
        int cost;
        int room_capacity_cost;
        int min_working_days_cost;
        int curriculum_compactness_cost;
        int room_stability_cost;
    } delta;
} swap_result;

void swap_iter_init(swap_iter *iter, const solution *sol);
void swap_iter_destroy(swap_iter *iter);
bool swap_iter_next(swap_iter *iter);

bool swap_move_is_effective(const swap_move *mv);
void swap_move_compute_helper(const solution *sol, swap_move *mv);
void swap_move_copy(swap_move *dest, const swap_move *src);
void swap_move_reverse(const swap_move *mv, swap_move *reverse_mv);

void swap_move_generate_random_raw(const solution *sol, swap_move *mv);
void swap_move_generate_random_extended(const solution *sol, swap_move *mv,
                                        bool require_effectiveness, bool require_feasibility);
void swap_move_generate_random_feasible_effective(const solution *sol, swap_move *mv);

void swap_predict(const solution *sol, const swap_move *move,
                  neighbourhood_predict_feasibility_strategy predict_feasibility,
                  neighbourhood_predict_cost_strategy predict_cost,
                  swap_result *result);
bool swap_perform(solution *sol, const swap_move *move,
                  neighbourhood_perform_strategy perform,
                  swap_result *result);

//bool swap_extended(solution *sol, const swap_move *move,
//                   neighbourhood_predict_feasibility_strategy predict_feasibility,
//                   neighbourhood_predict_cost_strategy predict_cost,
//                   neighbourhood_perform_strategy perform,
//                   swap_result *result);

#endif // SWAP_H