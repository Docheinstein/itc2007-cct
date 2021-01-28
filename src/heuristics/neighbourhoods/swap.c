#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/random_utils.h>
#include "swap.h"
#include "utils/array_utils.h"
#include "solution/solution.h"
#include "model/model.h"
#include "utils/assert_utils.h"



static bool check_lectures_constraint(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    MODEL(sol->model);
    const bool same_period = d1 == d2 && s1 == s2;
    const bool same_course = c1 == c2;

    if (c1 >= 0) {
        debug2("Check H1b (Lectures) (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (sol->sum_cds[INDEX3(c1, C, d2, D, s2, S)] - same_period - same_course > 0)
            return false;
    }

    return true;
}

static bool check_conflicts_curriculum_constraint(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    MODEL(sol->model);
    const bool same_period = d1 == d2 && s1 == s2;

    if (c1 >= 0) {
        int c1_n_curriculas;
        int * c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
        for (int cq = 0; cq < c1_n_curriculas; cq++) {
            const int q = c1_curriculas[cq];
            bool share_curricula = c2 >= 0 && model_share_curricula(model, c1, c2, q);
            debug2("Check H3a (Conflicts) (q=%d, d=%d, s=%d) -> %d", q, d2, s2, sol->sum_qds[INDEX3(q, Q, d2, D, s2, S)]);
            if (sol->sum_qds[INDEX3(q, Q, d2, D, s2, S)] - same_period - share_curricula > 0)
                return false;
        }
    }

    return true;
}

static bool check_conflicts_teacher_constraint(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    MODEL(sol->model);
    const bool same_teacher = c1 >= 0 && c2 >= 0 && model_same_teacher(model, c1, c2);
    const bool same_period = d1 == d2 && s1 == s2;

    if (c1 >= 0) {
        const int t1 = model->courses[c1].teacher->index;
        debug2("Check H3b (Conflicts) (t=%d, d=%d, s=%d)", t1, d2, s2);
        if (sol->sum_tds[INDEX3(t1, T, d2, D, s2, S)] - same_period - same_teacher > 0)
            return false;
    }

    return true;
}

static bool check_availabilities_constraint(
        const solution *sol, int c1, int d2, int s2) {
    if (c1 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (!model_course_is_available_on_period(sol->model, c1, d2, s2))
            return false;
    }

    return true;
}


static int compute_room_capacity_cost(
        const solution *sol, int c, int r_from, int r_to) {
    if (c < 0)
        return 0;

    int cost =
        MIN(0, sol->model->rooms[r_from].capacity - sol->model->courses[c].n_students) +
        MAX(0, sol->model->courses[c].n_students - sol->model->rooms[r_to].capacity);
    cost *= ROOM_CAPACITY_COST_FACTOR;

    debug2("RoomCapacity delta cost: %d", cost);
    return cost;
}

static int compute_min_working_days_cost(
        const solution *sol, int c_from, int d_from, int c_to, int d_to) {

    if (c_from < 0)
        return 0;
    if (c_from == c_to)
        return 0;

    MODEL(sol->model);

    int min_working_days = sol->model->courses[c_from].min_working_days;
    int prev_working_days = 0;
    int cur_working_days = 0;
    FOR_D {
        prev_working_days += MIN(1, sol->sum_cd[INDEX2(c_from, C, d, D)]);
        cur_working_days += MIN(1, sol->sum_cd[INDEX2(c_from, C, d, D)] - (d == d_from) + (d == d_to));
    }

    int cost =
            MIN(0, prev_working_days - min_working_days) +
            MAX(0, min_working_days - cur_working_days);
    cost *= MIN_WORKING_DAYS_COST_FACTOR;

    debug2("MinWorkingDays delta cost: %d", cost);
    return cost;
}

static int compute_room_stability_cost(
        const solution *sol, int c_from, int r_from, int c_to, int r_to) {
    if (c_from < 0)
        return 0;
    if (r_from == r_to)
        return 0;
    if (c_from == c_to)
        return 0;

    MODEL(sol->model);

    int prev_rooms = 0;
    int cur_rooms = 0;
    FOR_R {
        prev_rooms += MIN(1, sol->sum_cr[INDEX2(c_from, C, r, R)]);
        cur_rooms += MIN(1, sol->sum_cr[INDEX2(c_from, C, r, R)] - (r == r_from) + (r == r_to));
    };

    int cost = MAX(0, cur_rooms - 1) - MAX(0, prev_rooms - 1);
    cost *= ROOM_STABILITY_COST_FACTOR;

    debug2("RoomStability delta cost: %d", cost);
    return cost;
}

static int compute_curriculum_compactness_cost(
     const solution *sol, int c_from, int d_from, int s_from, int c_to, int d_to, int s_to) {
    if (c_from < 0)
        return 0;
    if (c_from == c_to)
        return 0;

    MODEL(sol->model);

    int cost = 0;

    int c_n_curriculas;
    int *c_curriculas = model_curriculas_of_course(model, c_from, &c_n_curriculas);

    for (int cq = 0; cq < c_n_curriculas; cq++) {
        int q = c_curriculas[cq];

        if (c_to >= 0 && model_share_curricula(model, c_to, c_from, q)) {
            continue; // swap of courses of the same curriculum has no cost
        }

#define Z(q, d, s) \
    ((s) >= 0 && (s) < S && sol->sum_qds[INDEX3(q, Q, d, D, s, S)])

#define Z_OUT_AFTER(q, d, s) \
    (!((d) == d_from && (s) == s_from) && \
    ((Z(q, d, s))))

#define Z_IN_BEFORE(q, d, s) \
    (!((d) == d_from && (s) == s_from) && \
    ((Z(q, d, s))))

#define Z_IN_AFTER(q, d, s) \
    (((s) == s_to && (d) == d_to) || \
    (!((d) == d_from && (s) == s_from) && \
    ((Z(q, d, s)))))

#define ALONE_OUT_BEFORE(q, d, s) \
    (Z(q, d, s) && !Z(q, d, (s) - 1) && !Z(q, d, (s) + 1))

#define ALONE_OUT_AFTER(q, d, s) \
    (Z_OUT_AFTER(q, d, s) && !Z_OUT_AFTER(q, d, (s) - 1) && !Z_OUT_AFTER(q, d, (s) + 1))

#define ALONE_IN_BEFORE(q, d, s) \
    (Z_IN_BEFORE(q, d, s) && !Z_IN_BEFORE(q, d, (s) - 1) && !Z_IN_BEFORE(q, d, (s) + 1))

#define ALONE_IN_AFTER(q, d, s) \
    (Z_IN_AFTER(q, d, s) && !Z_IN_AFTER(q, d, (s) - 1) && !Z_IN_AFTER(q, d, (s) + 1))

        int out_prev_cost_before = ALONE_OUT_BEFORE(q, d_from, s_from - 1);
        int out_itself_cost = ALONE_OUT_BEFORE(q, d_from, s_from);
        int out_next_cost_before = ALONE_OUT_BEFORE(q, d_from, s_from + 1);

        int out_prev_cost_after = ALONE_OUT_AFTER(q, d_from, s_from - 1);
        int out_next_cost_after = ALONE_OUT_AFTER(q, d_from, s_from + 1);

        int in_prev_cost_before = ALONE_IN_BEFORE(q, d_to, s_to - 1);
        int in_next_cost_before = ALONE_IN_BEFORE(q, d_to, s_to + 1);
        int in_prev_cost_after = ALONE_IN_AFTER(q, d_to, s_to - 1);
        int in_next_cost_after = ALONE_IN_AFTER(q, d_to, s_to + 1);

        int in_itself_cost = ALONE_IN_AFTER(q, d_to, s_to);

        int q_cost =
            (out_prev_cost_after - out_prev_cost_before) +
            (out_next_cost_after - out_next_cost_before) +
            (in_prev_cost_after - in_prev_cost_before) +
            (in_next_cost_after - in_next_cost_before) +
            (in_itself_cost - out_itself_cost);

        debug2("compute_curriculum_compactness_cost q=%d:%s\n"
                "   c_from=%d, d_from=%d, s_from=%d, c_to=%d, d_to=%d, s_to=%d\n"
                "   (out_prev_cost_after %d   -  out_prev_cost_before %d) +\n"
                "   (out_next_cost_after %d   -  out_next_cost_before %d) +\n"
                "   (in_prev_cost_after  %d   -  in_prev_cost_before  %d) +\n"
                "   (in_next_cost_after  %d   -  out_prev_cost_before %d) +\n"
                "   (in_itself_cost      %d   -  out_itself_cost      %d) = %d",
                q, model->curriculas[q].id,
                c_from, d_from, s_from, c_to, d_to, s_to,
                out_prev_cost_after, out_prev_cost_before,
                out_next_cost_after, out_next_cost_before,
                in_prev_cost_after, in_prev_cost_before,
                in_next_cost_after, in_next_cost_before,
                in_itself_cost, out_itself_cost,
                q_cost
        );


        if (s_from > 0)
            debug2("sol->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from - 1=%d, S)]=%d",
                  q, d_from, s_from - 1,
                  sol->sum_qds[INDEX3(q, Q, d_from, D, s_from - 1, S)]);
        debug2("sol->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from=%d, S)]=%d",
              q, d_from, s_from ,
              sol->sum_qds[INDEX3(q, Q, d_from, D, s_from, S)]);
        if (s_from < S - 1)
            debug2("sol->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from+1=%d, S)]=%d",
                  q, d_from, s_from + 1,
                  sol->sum_qds[INDEX3(q, Q, d_from, D, s_from + 1, S)]);
        if (s_from < S - 2)
            debug2("sol->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from+2=%d, S)]=%d",
                  q, d_from, s_from + 2,
                  sol->sum_qds[INDEX3(q, Q, d_from, D, s_from + 2, S)]);

        debug2("out_next_cost_after\n"
              "  Z_OUT_AFTER(q, d_from, s+1)=%d && !Z_OUT_AFTER(q, d_from, s)=%d && !Z_OUT_AFTER(q, d_from, s+2)=%d -> %d"
              ,
              Z_OUT_AFTER(q, d_from, s_from + 1), !Z_OUT_AFTER(q, d_from, (s_from)), !Z_OUT_AFTER(q, d_from, (s_from) + 2),
              out_next_cost_after);

        debug2("((Z(q, d_from, s_from+2))))= %d", Z(q, d_from, (s_from) + 2));
        debug2("(!((d_from) == d_from && (s_from+2) == s_from)= %d", (!((d_from) == d_from && (s_from+2) == s_from)));

#undef Z
#undef Z_OUT_AFTER
#undef Z_IN_BEFORE
#undef Z_IN_AFTER
#undef ALONE_OUT_BEFORE
#undef ALONE_OUT_AFTER
#undef ALONE_IN_BEFORE
#undef ALONE_IN_AFTER

        cost += q_cost;
    }

    cost *= CURRICULUM_COMPACTNESS_COST_FACTOR;

    debug2("CurriculumCompactness delta cost: %d", cost);

    return cost;
}


static bool swap_move_check_hard_constraints(const solution *sol,
                                             const swap_move *mv) {
    debug2("Check hard constraints of swap_extended (%d, %d %d, %d) <-> (%d, %d, %d, %d)",
           mv->helper.c1, mv->helper.r1, mv->d1, mv->helper.s1, mv->helper.c2, mv->r2, mv->d2, mv->s2);

    if (mv->helper.c1 == mv->helper.c2)
        return true; // nothing to do

    // Lectures
    if (!check_lectures_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;
    if (!check_lectures_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;

    // RoomOccupancy: no need to check since the swap replaces the room by design

    // Conflicts: curriculum
    if (!check_conflicts_curriculum_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;

    if (!check_conflicts_curriculum_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;

    // Conflicts: teacher
    if (!check_conflicts_teacher_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;

    if (!check_conflicts_teacher_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;

    // Availabilities
    if (!check_availabilities_constraint(
            sol, mv->helper.c1, mv->d2, mv->s2))
        return false;

    if (!check_availabilities_constraint(
            sol, mv->helper.c2, mv->helper.d1, mv->helper.s1))
        return false;

    return true;
}

static void swap_move_compute_cost(
        const solution *sol,
        const swap_move *mv,
        swap_result *result) {
    MODEL(sol->model);

    result->delta.room_capacity_cost = 0;
    result->delta.min_working_days_cost = 0;
    result->delta.curriculum_compactness_cost = 0;
    result->delta.room_stability_cost = 0;

    // Room capacity
    result->delta.room_capacity_cost +=
            compute_room_capacity_cost(
                    sol, mv->helper.c1, mv->helper.r1, mv->r2) +
                compute_room_capacity_cost(
                        sol, mv->helper.c2, mv->r2, mv->helper.r1);

    // MinWorkingDays
    result->delta.min_working_days_cost +=
            compute_min_working_days_cost(
                    sol, mv->helper.c1, mv->helper.d1, mv->helper.c2, mv->d2) +
                    compute_min_working_days_cost(
                            sol, mv->helper.c2, mv->d2, mv->helper.c1, mv->helper.d1);

    // CurriculumCompactness
    result->delta.curriculum_compactness_cost +=
            compute_curriculum_compactness_cost(
                    sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2) +
                    compute_curriculum_compactness_cost(
                            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1);

    // RoomStability
    result->delta.room_stability_cost +=
            compute_room_stability_cost(
                    sol, mv->helper.c1, mv->helper.r1, mv->helper.c2, mv->r2) +
                    compute_room_stability_cost(
                            sol, mv->helper.c2, mv->r2, mv->helper.c1, mv->helper.r1);

    result->delta.cost =
            result->delta.room_capacity_cost +
            result->delta.min_working_days_cost +
            result->delta.curriculum_compactness_cost +
            result->delta.room_stability_cost;

    debug2("swap_move_compute_cost cost = %d", result->delta_cost);
}


static void swap_move_do(solution *sol, const swap_move *mv) {
//    char tmp[92];
//    snprintf(tmp, 92, "%*d,%*d,%*d,%*d | %*d,%*d,%*d,%*d\n",
//            2, c1, 2, r1, 2, d1, 2, s1, 2, c2, 2, r2, 2, d2, 2, s2);
//    fileappend("/tmp/moves.txt", tmp);

    // Update timetable
    if (mv->l1 >= 0)
        solution_set_lecture_assignment(sol, mv->l1, mv->r2, mv->d2, mv->s2);
    if (mv->helper.l2 >= 0)
        solution_set_lecture_assignment(sol, mv->helper.l2, mv->helper.r1, mv->helper.d1, mv->helper.s1);
}

void swap_iter_init(swap_iter *iter, const solution *sol) {
    iter->solution = sol;
    iter->current.l1 = iter->current.r2 = iter->current.d2 = iter->current.s2;
    iter->end = false;
    iter->i = iter->ii = 0;
}

void swap_iter_destroy(swap_iter *iter) {}

static void swap_move_compute_helper(const solution *sol,
                                     swap_move *mv) {
    MODEL(sol->model);
    mv->helper.c1 = sol->model->lectures[mv->l1].course->index;
    solution_get_lecture_assignment(sol, mv->l1, &mv->helper.r1, &mv->helper.d1, &mv->helper.s1);

    mv->helper.l2 = sol->l_rds[INDEX3(mv->r2, R, mv->d2, D, mv->s2, S)];
    if (mv->helper.l2 >= 0)
        mv->helper.c2 = sol->model->lectures[mv->helper.l2].course->index;
}

static bool swap_move_is_effective(const swap_move *mv) {
    return mv->helper.c1 != mv->helper.c2;
}

static bool swap_iter_skip(const swap_iter *iter) {
    return iter->current.helper.c1 <= iter->current.helper.c2;
}

bool swap_iter_next(swap_iter *iter,
                    swap_move *move) {
    if (iter->end)
        return false;

    MODEL(iter->solution->model);

    do {
        iter->current.s2 = (iter->current.s2 + 1) % S;
        if (!iter->current.s2) {
            iter->current.d2 = (iter->current.d2 + 1) % D;
            if (!iter->current.d2) {
                iter->current.r2 = (iter->current.r2 + 1) % R;
                if (!iter->current.r2) {
                    iter->current.l1 = (iter->current.l1 + 1) % L;
                    if (!iter->current.l1) {
                        debug2("Iter exhausted after %d iters", iter->i);
                        iter->end = true;
                        return false;
                    }
                    swap_move_compute_helper(iter->solution, &iter->current);
                }
            }
        }
        iter->ii++;
    } while (swap_iter_skip(iter));
    assert(swap_move_is_effective(&iter->current));

    iter->i++;

    swap_move_copy(move, &iter->current);

    return true;
}


void swap_predict(const solution *sol, const swap_move *move,
                  neighbourhood_predict_strategy predict_feasibility,
                  neighbourhood_predict_strategy predict_cost,
                  swap_result *result) {

    if (predict_feasibility == NEIGHBOURHOOD_PREDICT_ALWAYS)
        if (result)
            result->feasible = swap_move_check_hard_constraints(sol, move);

    if (predict_cost == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_cost == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE && result->feasible))
        swap_move_compute_cost(sol, move, result);
}

bool swap_perform(solution *sol, const swap_move *move,
                  neighbourhood_perform_strategy perform,
                  swap_result *result) {
    if (perform == NEIGHBOURHOOD_PERFORM_ALWAYS ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE && result->feasible) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_BETTER && result->delta.cost < 0) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER && result->feasible && result->delta.cost < 0)) {
        swap_move_do(sol, move);
        return true;
    }
    return false;
}

bool swap_extended(solution *sol, const swap_move *mv,
                   neighbourhood_predict_strategy predict_feasibility,
                   neighbourhood_predict_strategy predict_cost,
                   neighbourhood_perform_strategy perform,
                   swap_result *result) {
    swap_predict(sol, mv, predict_feasibility, predict_cost, result);
    return swap_perform(sol, mv, perform, result);
}

void swap_move_generate_random(const solution *sol, swap_move *mv, bool require_feasible) {
    MODEL(sol->model);

    swap_result result;

    do {
        do {
            mv->l1 = rand_int_range(0, L);
            mv->r2 = rand_int_range(0, R);
            mv->d2 = rand_int_range(0, D);
            mv->s2 = rand_int_range(0, S);
            swap_move_compute_helper(sol, mv);
        } while (!swap_move_is_effective(mv));

        if (require_feasible)
            swap_predict(sol, mv,
                         NEIGHBOURHOOD_PREDICT_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_NEVER,
                         &result);
    } while(!require_feasible || result.feasible);
}

void swap_move_copy(swap_move *dest, const swap_move *src) {
    memcpy(dest, src, sizeof(swap_move));
}

