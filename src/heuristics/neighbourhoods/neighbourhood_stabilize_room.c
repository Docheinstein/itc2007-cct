#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include <config.h>
#include <utils/mem_utils.h>
#include <renderer.h>
#include "neighbourhood_stabilize_room.h"
#include "utils/array_utils.h"
#include "solution.h"
#include "model.h"
#include "neighbourhood_swap.h"


void neighbourhood_stabilize_room_move_copy(neighbourhood_stabilize_room_move *dest,
                                            const neighbourhood_stabilize_room_move *src) {
    dest->c1 = src->c1;
    dest->r2 = src->r2;
}

void neighbourhood_stabilize_room_iter_init(
        neighbourhood_stabilize_room_iter *iter,
        solution *sol) {
    iter->solution = sol;
    iter->end = false;
    iter->current.c1 = iter->current.r2 = -1;
}

void neighbourhood_stabilize_room_iter_destroy(neighbourhood_stabilize_room_iter *iter) {}

bool neighbourhood_stabilize_room_iter_next(neighbourhood_stabilize_room_iter *iter,
                                            neighbourhood_stabilize_room_move *move) {
    if (iter->end)
        return false;

    CRDSQT(iter->solution->model)

    iter->current.r2 = (iter->current.r2 + 1) % R;
    if (!iter->current.r2) {
        iter->current.c1 = (iter->current.c1 + 1);
    }

    neighbourhood_stabilize_room_move_copy(move, &iter->current);

    debug2("neighbourhood_stabilize_room_iter_next -> %d %d", move->c1, move->r2);
    return iter->current.c1 < C;
}

static void compute_neighbourhood_stabilize_room_cost(
        solution *sol, neighbourhood_stabilize_room_move *mv,
        neighbourhood_stabilize_room_result *result) {
    CRDSQT(sol->model)
    const solution_helper *helper = solution_get_helper(sol);

    debug2("compute_neighbourhood_stabilize_room_cost c=%d:%s r=%d:%s",
          mv->c1, sol->model->courses[mv->c1].id,
          mv->r2, sol->model->rooms[mv->r2].id);

    int n_students = sol->model->courses[mv->c1].n_students;

    int *sum_cr_future = mallocx(C * R, C * R * sizeof(int));
    memcpy(sum_cr_future, helper->sum_cr, C * R * sizeof(int));

    // Use just one room for course c1, by design
    FOR_R
        sum_cr_future[INDEX2(mv->c1, C, r, R)] = 0;
    sum_cr_future[INDEX2(mv->c1, C, mv->r2, R)] = sol->model->courses[mv->c1].n_lectures;

    int room_stability_cost = 0;
    int room_capacity_cost = 0;

    // RoomStability
    FOR_D {
        FOR_S {
            int r1 = solution_get_helper(sol)->r_cds[INDEX3(mv->c1, C, d, D, s, S)];
            if (r1 < 0)
                continue; // course c1 not scheduled in this time slot

            int c2 = solution_get_helper(sol)->c_rds[INDEX3(mv->r2, C, d, D, s, S)];
            if (mv->c1 == c2)
                continue; // course c1 already in room r2

            int rc1 =
                MIN(0, sol->model->rooms[r1].capacity - n_students) +
                MAX(0, n_students - sol->model->rooms[mv->r2].capacity);
            debug2("move %s (%d) from %s (%d) to %s (%d) in (d=%d,s=%d) has rc_cost=%d",
                  sol->model->courses[mv->c1].id, sol->model->courses[mv->c1].n_students,
                  sol->model->rooms[r1].id, sol->model->rooms[r1].capacity,
                  sol->model->rooms[mv->r2].id, sol->model->rooms[mv->r2].capacity,
                  d, s,
                  rc1);
            room_capacity_cost += rc1;

            if (c2 >= 0) {
                // There is a course scheduled in the target room
                sum_cr_future[INDEX2(c2, C, mv->r2, R)] -= 1; // not scheduled anymore in r2
                sum_cr_future[INDEX2(c2, C, r1, R)] += 1; // scheduled in r1

                int rc2 =
                    MIN(0, sol->model->rooms[mv->r2].capacity - sol->model->courses[c2].n_students) +
                    MAX(0, sol->model->courses[c2].n_students - sol->model->rooms[r1].capacity);
                debug2("move %s (%d) from %s (%d) to %s (%d) in (d=%d,s=%d) has rc_cost=%d",
                  sol->model->courses[c2].id, sol->model->courses[c2].n_students,
                  sol->model->rooms[mv->r2].id, sol->model->rooms[mv->r2].capacity,
                  sol->model->rooms[r1].id, sol->model->rooms[r1].capacity,
                  d, s,
                  rc2);
                room_capacity_cost += rc2;
            }
        }
    };

    FOR_C {
        int prev_rooms = 0;
        int cur_rooms = 0;
        FOR_R {
            prev_rooms += MIN(1, helper->sum_cr[INDEX2(c, C, r, R)]);
            cur_rooms += MIN(1, sum_cr_future[INDEX2(c, C, r, R)]);
        }

        int rs_delta = (cur_rooms - prev_rooms);
        if (rs_delta != 0)
            debug2("rs_delta(c=%s)=%d", sol->model->courses[c].id, rs_delta);
        room_stability_cost += rs_delta;

    };

    result->delta_cost_room_capacity = room_capacity_cost * ROOM_CAPACITY_COST;
    result->delta_cost_min_working_days = 0;
    result->delta_cost_curriculum_compactness = 0;
    result->delta_cost_room_stability = room_stability_cost * ROOM_STABILITY_COST;
    result->delta_cost = result->delta_cost_room_capacity + result->delta_cost_room_stability;


    debug2("compute_neighbourhood_stabilize_room_cost c=%d:%s r=%d:%s -> %d (rc=%d, rs=%d)",
          mv->c1, sol->model->courses[mv->c1].id,
          mv->r2, sol->model->rooms[mv->r2].id,
          result->delta_cost,
          result->delta_cost_room_capacity,
          result->delta_cost_room_stability);

    free(sum_cr_future);
}

static void do_neighbourhood_stabilize_room(solution *sol, int c1, int r2) {
    CRDSQT(sol->model)

    debug2("neighbourhood_stabilize_room c=%d:%s r=%d:%s",
          c1, sol->model->courses[c1].id,
          r2, sol->model->rooms[r2].id);

    const solution_helper *helper = solution_get_helper(sol);
    neighbourhood_swap_move swap_move;
    swap_move.c1 = c1;
    swap_move.r2 = r2;

    FOR_D {
        swap_move.d1 = swap_move.d2 = d;
        FOR_S {
            swap_move.s1 = swap_move.s2 = s;

            int r1 = solution_get_helper(sol)->r_cds[INDEX3(c1, C, d, D, s, S)];
            if (r1 < 0)
                continue;

            swap_move.r1 = r1;
            swap_move.c2 = helper->c_rds[INDEX3(swap_move.r2, R, swap_move.d2, D, swap_move.s2, S)];
            swap_move.l1 = helper->lectures_crds[INDEX4(swap_move.c1, C, swap_move.r1, R, swap_move.d1, D, swap_move.s1, S)];
            swap_move.l2 = helper->lectures_crds[INDEX4(swap_move.c2, C, swap_move.r2, R, swap_move.d2, D, swap_move.s2, S)];

            neighbourhood_swap(sol, &swap_move,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PERFORM_ALWAYS,
               NULL);
        }
    }
}

static void compute_neighbourhood_stabilize_room_fingerprint_diff(
        solution *sol, neighbourhood_stabilize_room_move *mv,
        neighbourhood_stabilize_room_result *result) {
    CRDSQT(sol->model)

    solution_fingerprint_init(&result->fingerprint_plus);
    solution_fingerprint_init(&result->fingerprint_minus);

    debug2("neighbourhood_stabilize_room c=%d:%s r=%d:%s",
            mv->c1, sol->model->courses[mv->c1].id,
            mv->r2, sol->model->rooms[mv->r2].id);

    neighbourhood_swap_move swap_move;
    swap_move.c1 = mv->c1;
    swap_move.r2 = mv->r2;

    FOR_D {
        swap_move.d1 = swap_move.d2 = d;
        FOR_S {
            swap_move.s1 = swap_move.s2 = s;

            int r1 = solution_get_helper(sol)->r_cds[INDEX3(mv->c1, C, d, D, s, S)];
            if (r1 < 0)
                continue;

            swap_move.r1 = r1;
            neighbourhood_swap_result swap_result;

            neighbourhood_swap(sol, &swap_move,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_NEVER,
               NEIGHBOURHOOD_PREDICT_ALWAYS,
               NEIGHBOURHOOD_PERFORM_NEVER,
               &swap_result);

            result->fingerprint_plus.sum += swap_result.fingerprint_plus.sum;
            result->fingerprint_plus.xor ^= swap_result.fingerprint_plus.xor;
            result->fingerprint_minus.sum += swap_result.fingerprint_minus.sum;
            result->fingerprint_minus.xor ^= swap_result.fingerprint_minus.xor;
        }
    }
}

bool neighbourhood_stabilize_room(solution *sol,
                                  neighbourhood_stabilize_room_move *mv,
                                  neighbourhood_prediction_strategy predict_cost,
                                  neighbourhood_prediction_strategy predict_fingerprint,
                                  neighbourhood_performing_strategy perform,
                                  neighbourhood_stabilize_room_result *result) {
    CRDSQT(sol->model)

    if (predict_cost == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_cost == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE))
        compute_neighbourhood_stabilize_room_cost(sol, mv, result);

    if (predict_fingerprint == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_fingerprint == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE))
        compute_neighbourhood_stabilize_room_fingerprint_diff(sol, mv, result);

    if (perform == NEIGHBOURHOOD_PERFORM_ALWAYS ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_BETTER && result->delta_cost < 0) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER && result->delta_cost < 0)) {
        int cost = solution_cost(sol);
        do_neighbourhood_stabilize_room(sol, mv->c1, mv->r2);
#if debug2
        if ((predict_cost == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_cost == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE)) &&
        cost + result->delta_cost != solution_cost(sol)) {
            eprint("Expected cost %d, found %d", cost + result->delta_cost, solution_cost(sol));
            exit(32);
        }
#endif
        return true;
    }

    return false;
}