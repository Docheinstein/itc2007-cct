#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include <utils/def_utils.h>
#include "neighbourhood_swap.h"
#include "utils/array_utils.h"
#include "solution.h"
#include "model.h"
#include "config.h"
#include "utils/assert_utils.h"


bool neighbourhood_swap_move_same_room(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2) {
    return
            (
                    m1->r2 == m2->r2
            ) || (
                    m1->r2 == m2->r1
            );
}

bool neighbourhood_swap_move_same_period(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2) {
    return
            (
                    m1->d1 == m2->d1 &&
                    m1->s1 == m2->s1 &&
                    m1->r2 == m2->r2 &&
                    m1->d2 == m2->d2
            ) || (
                    m1->d1 == m2->d2 &&
                    m1->s1 == m2->s2 &&
                    m1->r2 == m2->r1 &&
                    m1->d2 == m2->d1
            );
}

bool neighbourhood_swap_move_same_course(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2) {
    return
            (
                    m1->c1 == m2->c1
            ) || (
                    m1->c1 == m2->_c2
            );
}

bool neighbourhood_swap_move_equal(const neighbourhood_swap_move *m1, const neighbourhood_swap_move *m2) {
    return
            (
                    m1->c1 == m2->c1 &&
                    m1->r1 == m2->r1 &&
                    m1->d1 == m2->d1 &&
                    m1->s1 == m2->s1 &&
                    m1->_c2 == m2->_c2 &&
                    m1->r2 == m2->r2 &&
                    m1->d2 == m2->d2 &&
                    m1->s2 == m2->s2
            ) || (
                    m1->c1 == m2->_c2 &&
                    m1->r1 == m2->r2 &&
                    m1->d1 == m2->d2 &&
                    m1->s1 == m2->s2 &&
                    m1->_c2 == m2->c1 &&
                    m1->r2 == m2->r1 &&
                    m1->d2 == m2->d1 &&
                    m1->s2 == m2->s1
            );
}

void neighbourhood_swap_move_copy(neighbourhood_swap_move *dest, const neighbourhood_swap_move *src) {
    dest->c1 = src->c1;
    dest->r1 = src->r1;
    dest->d1 = src->d1;
    dest->s1 = src->s1;
    dest->_c2 = src->_c2;
    dest->r2 = src->r2;
    dest->d2 = src->d2;
    dest->s2 = src->s2;
}

static bool neighbourhood_swap_move_is_effective(const neighbourhood_swap_move *mv) {
    return
        mv->r1 != mv->r2 ||
        mv->d1 != mv->d2 ||
        mv->s1 != mv->s2;
}


void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, solution *sol) {
    iter->solution = sol;
    iter->end = false;
    iter->rds_index = -1;
    iter->current.c1 = iter->current.r1 = iter->current.d1 = iter->current.s1 = -1;
    iter->current.r2 = iter->current.d2 = iter->current.s2 = -1;

#ifdef DEBUG2
    debug2("neighbourhood_swap_iter_init for solution");
    print_solution(sol, stdout);
#endif
}

void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter) {}

bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter,
                                  neighbourhood_swap_move *move) {
    if (iter->end)
        return false;

    CRDSQT(iter->solution->model)
    const int RDS = R * D * S;
    const solution_helper *helper = solution_get_helper(iter->solution);

    do {
        iter->current.s2 = (iter->current.s2 + 1) % S;
        if (!iter->current.s2) {
            iter->current.d2 = (iter->current.d2 + 1) % D;
            if (!iter->current.d2) {
                iter->current.r2 = (iter->current.r2 + 1) % R;
                if (!iter->current.r2) {

                    // Find next assignment
                    do {
                        iter->rds_index++;
                    } while (iter->rds_index < RDS && (helper->c_rds[iter->rds_index]) < 0);

                    if (iter->rds_index < RDS) {
                        iter->current.c1 = helper->c_rds[iter->rds_index];
                        iter->current.r1 = RINDEX3_0(iter->rds_index, R, D, S);
                        iter->current.d1 = RINDEX3_1(iter->rds_index, R, D, S);
                        iter->current.s1 = RINDEX3_2(iter->rds_index, R, D, S);
                        debug2("Found next assignment (%d %d %d %d)",
                              iter->current.c1, iter->current.r1, iter->current.d1, iter->current.s1);
                    } else {
                        debug2("No assignment found, iter exhausted");
                        iter->end = true;
                        return false;
                    }
                }
            }
        }
    } while (!neighbourhood_swap_move_is_effective(&iter->current));


    neighbourhood_swap_move_copy(move, &iter->current);

    return true;
}

static bool check_neighbourhood_swap_lectures_constraint(
        solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    CRDSQT(sol->model);
    const solution_helper *helper = solution_get_helper(sol);
    const bool same_period = d1 == d2 && s1 == s2;
    const bool same_course = c1 == c2;

    if (c1 >= 0) {
        debug2("Check H1b (Lectures) (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (helper->sum_cds[INDEX3(c1, C, d2, D, s2, S)] - same_period - same_course > 0)
            return false;
    }

    return true;
}

static bool check_neighbourhood_swap_conflicts_curriculum_constraint(
        solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    CRDSQT(sol->model);
    const solution_helper *helper = solution_get_helper(sol);
    const model *model = sol->model;
    const bool same_period = d1 == d2 && s1 == s2;

    if (c1 >= 0) {
        int c1_n_curriculas;
        int * c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
        for (int cq = 0; cq < c1_n_curriculas; cq++) {
            const int q = c1_curriculas[cq];
            bool share_curricula = c2 >= 0 && model_share_curricula(model, c1, c2, q);
            debug2("Check H3a (Conflicts) (q=%d, d=%d, s=%d) -> %d", q, d2, s2, helper->sum_qds[INDEX3(q, Q, d2, D, s2, S)]);
            if (helper->sum_qds[INDEX3(q, Q, d2, D, s2, S)] - same_period - share_curricula > 0)
                return false;
        }
    }

    return true;
}

static bool check_neighbourhood_swap_conflicts_teacher_constraint(
        solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    CRDSQT(sol->model);
    const solution_helper *helper = solution_get_helper(sol);
    const model *model = sol->model;
    const bool same_teacher = c1 >= 0 && c2 >= 0 && model_same_teacher(model, c1, c2);
    const bool same_period = d1 == d2 && s1 == s2;

    if (c1 >= 0) {
        const int t1 = model->courses[c1].teacher->index;
        debug2("Check H3b (Conflicts) (t=%d, d=%d, s=%d)", t1, d2, s2);
        if (helper->sum_tds[INDEX3(t1, T, d2, D, s2, S)] - same_period - same_teacher > 0)
            return false;
    }


    return true;
}

static bool check_neighbourhood_swap_availabilities_constraint(
        solution *sol, int c1, int d2, int s2) {
    if (c1 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (!model_course_is_available_on_period(sol->model, c1, d2, s2))
            return false;
    }

    return true;
}

static bool check_neighbourhood_swap_hard_constraints(solution *sol,
                                                      const neighbourhood_swap_move *mv) {
    debug2("Check hard constraints of swap (%d, %d %d, %d) <-> (%d, %d, %d, %d)",
           mv->c1, mv->r1, mv->d1, mv->s1, mv->_c2, mv->r2, mv->d2, mv->s2);

    if (mv->d1 == mv->d2 && mv->s1 == mv->s2 && mv->c1 == mv->_c2 && mv->r1 == mv->r2)
        return true; // nothing to do

    // Lectures
    if (!check_neighbourhood_swap_lectures_constraint(
            sol, mv->c1, mv->d1, mv->s1, mv->_c2, mv->d2, mv->s2))
        return false;
    if (!check_neighbourhood_swap_lectures_constraint(
            sol, mv->_c2, mv->d2, mv->s2, mv->c1, mv->d1, mv->s1))
        return false;

    // RoomOccupancy: no need to check since the swap replaces the room by design

    // Conflicts: curriculum
    if (!check_neighbourhood_swap_conflicts_curriculum_constraint(
            sol, mv->c1, mv->d1, mv->s1, mv->_c2, mv->d2, mv->s2))
        return false;

    if (!check_neighbourhood_swap_conflicts_curriculum_constraint(
            sol, mv->_c2, mv->d2, mv->s2, mv->c1, mv->d1, mv->s1))
        return false;

    // Conflicts: teacher
    if (!check_neighbourhood_swap_conflicts_teacher_constraint(
            sol, mv->c1, mv->d1, mv->s1, mv->_c2, mv->d2, mv->s2))
        return false;

    if (!check_neighbourhood_swap_conflicts_teacher_constraint(
            sol, mv->_c2, mv->d2, mv->s2, mv->c1, mv->d1, mv->s1))
        return false;

    // Availabilities
    if (!check_neighbourhood_swap_availabilities_constraint(
            sol, mv->c1, mv->d2, mv->s2))
        return false;

    if (!check_neighbourhood_swap_availabilities_constraint(
            sol, mv->_c2, mv->d1, mv->s1))
        return false;

    return true;
}

static void do_neighbourhood_swap(
        solution *sol,
        int c1, int r1, int d1, int s1,
        int c2, int r2, int d2, int s2) {
    // Swap assignments
    debug2("do_neighbourhood_swap(%d, %d, %d, %d <-> %d, %d, %d, %d)",
          c1, r1, d1, s1, c2, r2, d2, s2);
    if (c1 >= 0)
        solution_set(sol, c1, r1, d1, s1, false);
    if (c2 >= 0)
        solution_set(sol, c2, r2, d2, s2, false);
    if (c1 >= 0)
        solution_set(sol, c1, r2, d2, s2, true);
    if (c2 >= 0)
        solution_set(sol, c2, r1, d1, s1, true);
}

static int compute_neighbourhood_swap_room_capacity_cost(
        solution *sol, int c, int r_from, int r_to) {
    if (c < 0)
        return 0;

    int cost =
        MIN(0, sol->model->rooms[r_from].capacity - sol->model->courses[c].n_students) +
        MAX(0, sol->model->courses[c].n_students - sol->model->rooms[r_to].capacity);
    cost *= ROOM_CAPACITY_COST;

    debug2("RoomCapacity delta cost: %d", cost);
    return cost;
}

static int compute_neighbourhood_swap_min_working_days_cost(
        solution *sol, int c_from, int d_from, int c_to, int d_to) {

    if (c_from < 0)
        return 0;
    if (c_from == c_to)
        return 0;

    CRDSQT(sol->model)
    const solution_helper *helper = solution_get_helper(sol);

    int min_working_days = sol->model->courses[c_from].min_working_days;
    int prev_working_days = 0;
    int cur_working_days = 0;
    FOR_D {
        prev_working_days += MIN(1, helper->sum_cd[INDEX2(c_from, C, d, D)]);
        cur_working_days += MIN(1, helper->sum_cd[INDEX2(c_from, C, d, D)] - (d == d_from) + (d == d_to));
    }

    int cost =
            MIN(0, prev_working_days - min_working_days) +
            MAX(0, min_working_days - cur_working_days);
    cost *= MIN_WORKING_DAYS_COST;

    debug2("MinWorkingDays delta cost: %d", cost);
    return cost;
}

static int compute_neighbourhood_swap_room_stability_cost(
        solution *sol, int c_from, int r_from, int c_to, int r_to) {
    if (c_from < 0)
        return 0;
    if (r_from == r_to)
        return 0;
    if (c_from == c_to)
        return 0;

    CRDSQT(sol->model)
    const solution_helper *helper = solution_get_helper(sol);

    int prev_rooms = 0;
    int cur_rooms = 0;
    FOR_R {
        prev_rooms += MIN(1, helper->sum_cr[INDEX2(c_from, C, r, R)]);
        cur_rooms += MIN(1, helper->sum_cr[INDEX2(c_from, C, r, R)] - (r == r_from) + (r == r_to));
    };

    int cost = MAX(0, cur_rooms - 1) - MAX(0, prev_rooms - 1);
    cost *= ROOM_STABILITY_COST;

    debug2("RoomStability delta cost: %d", cost);
    return cost;
}

static int compute_neighbourhood_swap_curriculum_compactness_cost(
     solution *sol, int c_from, int d_from, int s_from, int c_to, int d_to, int s_to) {
    if (c_from < 0)
        return 0;
    if (c_from == c_to)
        return 0;

    CRDSQT(sol->model)
    const model *model = sol->model;
    const solution_helper *helper = solution_get_helper(sol);

    int cost = 0;

    int c_n_curriculas;
    int *c_curriculas = model_curriculas_of_course(model, c_from, &c_n_curriculas);

    for (int cq = 0; cq < c_n_curriculas; cq++) {
        int q = c_curriculas[cq];

        if (c_to >= 0 && model_share_curricula(model, c_to, c_from, q)) {
            continue; // swap of courses of the same curriculum has no cost
        }

#define Z(q, d, s) \
    ((s) >= 0 && (s) < S && helper->sum_qds[INDEX3(q, Q, d, D, s, S)])

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

        debug2("compute_neighbourhood_swap_curriculum_compactness_cost q=%d:%s\n"
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
            debug2("helper->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from - 1=%d, S)]=%d",
                  q, d_from, s_from - 1,
                  helper->sum_qds[INDEX3(q, Q, d_from, D, s_from - 1, S)]);
        debug2("helper->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from=%d, S)]=%d",
              q, d_from, s_from ,
              helper->sum_qds[INDEX3(q, Q, d_from, D, s_from, S)]);
        if (s_from < S - 1)
            debug2("helper->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from+1=%d, S)]=%d",
                  q, d_from, s_from + 1,
                  helper->sum_qds[INDEX3(q, Q, d_from, D, s_from + 1, S)]);
        if (s_from < S - 2)
            debug2("helper->sum_qds[INDEX3(q=%d, Q, d_from=%d, D, s_from+2=%d, S)]=%d",
                  q, d_from, s_from + 2,
                  helper->sum_qds[INDEX3(q, Q, d_from, D, s_from + 2, S)]);

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

    cost *= CURRICULUM_COMPACTNESS_COST;

    debug2("CurriculumCompactness delta cost: %d", cost);

    return cost;
}

static void compute_neighbourhood_swap_cost(
        solution *sol,
        const neighbourhood_swap_move *mv,
        neighbourhood_swap_result *result) {
    CRDSQT(sol->model)

    debug2("compute_neighbourhood_swap_cost (c=%d:%s (r=%d:%s d=%d s=%d) (r=%d:%s, d=%d, s=%d))",
            mv->c1, sol->model->courses[mv->c1].id,
            mv->r1, sol->model->rooms[mv->r1].id, mv->d1, mv->s1,
            mv->r2, sol->model->rooms[mv->r2].id, mv->d2, mv->s2);

    result->delta_cost_room_capacity = 0;
    result->delta_cost_min_working_days = 0;
    result->delta_cost_curriculum_compactness = 0;
    result->delta_cost_room_stability = 0;

    // Room capacity
    result->delta_cost_room_capacity +=
        compute_neighbourhood_swap_room_capacity_cost(
                sol, mv->c1, mv->r1, mv->r2) +
        compute_neighbourhood_swap_room_capacity_cost(
                sol, mv->_c2, mv->r2, mv->r1);

    // MinWorkingDays
    result->delta_cost_min_working_days +=
            compute_neighbourhood_swap_min_working_days_cost(
                    sol, mv->c1, mv->d1, mv->_c2, mv->d2) +
            compute_neighbourhood_swap_min_working_days_cost(
                    sol, mv->_c2, mv->d2, mv->c1, mv->d1);

    // CurriculumCompactness
    result->delta_cost_curriculum_compactness +=
            compute_neighbourhood_swap_curriculum_compactness_cost(
                    sol, mv->c1, mv->d1, mv->s1, mv->_c2, mv->d2, mv->s2) +
            compute_neighbourhood_swap_curriculum_compactness_cost(
                    sol, mv->_c2, mv->d2, mv->s2, mv->c1, mv->d1, mv->s1);

    // RoomStability
    result->delta_cost_room_stability +=
            compute_neighbourhood_swap_room_stability_cost(
                    sol, mv->c1, mv->r1, mv->_c2, mv->r2) +
            compute_neighbourhood_swap_room_stability_cost(
                    sol, mv->_c2, mv->r2, mv->c1, mv->r1);

    result->delta_cost =
            result->delta_cost_room_capacity +
            result->delta_cost_min_working_days +
            result->delta_cost_curriculum_compactness +
            result->delta_cost_room_stability;

    debug2("compute_neighbourhood_swap_cost cost = %d", result->delta_cost);
}

static void compute_neighbourhood_swap_fingerprint_diff(
        solution *sol,
        const neighbourhood_swap_move *mv,
        neighbourhood_swap_result *result) {
    CRDSQT(sol->model);
    debug2("compute_neighbourhood_swap_cost_fingerprint_diff (c=%d:%s (r=%d:%s d=%d s=%d) (r=%d:%s, d=%d, s=%d)) [c2=%d]",
            mv->c1, sol->model->courses[mv->c1].id,
            mv->r1, sol->model->rooms[mv->r1].id, mv->d1, mv->s1,
            mv->r2, sol->model->rooms[mv->r2].id, mv->d2, mv->s2, mv->_c2);

#define FINGERPRINT_PLUS(i) if (!solution_get_at(sol, (i))) solution_fingerprint_add(&result->fingerprint_plus, i)
#define FINGERPRINT_MINUS(i) if (solution_get_at(sol, (i))) solution_fingerprint_add(&result->fingerprint_minus, i)

    int i1 = INDEX4(mv->c1, C, mv->r1, R, mv->d1, D, mv->s1, S);
    int i2 = INDEX4(mv->_c2, C, mv->r2, R, mv->d2, D, mv->s2, S);
    int i3 = INDEX4(mv->c1, C, mv->r2, R, mv->d2, D, mv->s2, S);
    int i4 = INDEX4(mv->_c2, C, mv->r1, R, mv->d1, D, mv->s1, S);
    solution_fingerprint_init(&result->fingerprint_plus);
    solution_fingerprint_init(&result->fingerprint_minus);

    debug2("i1=%d, i2=%d, i3=%d, i4=%d", i1, i2, i3, i4);
    debug2("sol[i1]=%d sol[i2]=%d sol[i3]=%d sol[i4]=%d",
          solution_get_at(sol, i1), solution_get_at(sol, i2),
          solution_get_at(sol, i3), solution_get_at(sol, i4));

    if (mv->c1 >= 0) {
        if (i1 != i3 && i1 != i4)
            FINGERPRINT_MINUS(i1);
        FINGERPRINT_PLUS(i3);
    }

    if (mv->_c2 >= 0) {
        if (i2 != i3 && i2 != i4)
            FINGERPRINT_MINUS(i2);
        FINGERPRINT_PLUS(i4);
    }

#undef FINGERPRINT_PLUS
#undef FINGERPRINT_MINUS

    debug2("fingerprint plus = (sum=%llu, prod=%llu)", result->fingerprint_plus.sum, result->fingerprint_plus.xor);
    debug2("fingerprint minus = (sum=%llu, prod=%llu)", result->fingerprint_minus.sum, result->fingerprint_minus.xor);
}

bool neighbourhood_swap(solution *sol,
                        neighbourhood_swap_move *mv,
                        neighbourhood_prediction_strategy predict_feasibility,
                        neighbourhood_prediction_strategy predict_cost,
                        neighbourhood_prediction_strategy predict_fingeprint,
                        neighbourhood_performing_strategy perform,
                        neighbourhood_swap_result *result) {
    CRDSQT(sol->model)

    assert(solution_get(sol, mv->c1, mv->r1, mv->d1, mv->s1));

    mv->_c2 = solution_get_helper(sol)->c_rds[INDEX3(mv->r2, R, mv->d2, D, mv->s2, S)];

    debug2("neighbourhood_swap (c=%d:%s (r=%d:%s d=%d s=%d) (r=%d:%s, d=%d, s=%d)) [c2=%d]",
            mv->c1, sol->model->courses[mv->c1].id,
            mv->r1, sol->model->rooms[mv->r1].id, mv->d1, mv->s1,
            mv->r2, sol->model->rooms[mv->r2].id, mv->d2, mv->s2, mv->_c2);

    if (predict_feasibility == NEIGHBOURHOOD_PREDICT_ALWAYS)
        if (result)
            result->feasible = check_neighbourhood_swap_hard_constraints(sol, mv);

    if (predict_cost == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_cost == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE && result->feasible))
        compute_neighbourhood_swap_cost(sol, mv, result);

    if (predict_fingeprint == NEIGHBOURHOOD_PREDICT_ALWAYS ||
        (predict_fingeprint == NEIGHBOURHOOD_PREDICT_IF_FEASIBLE && result->feasible))
        compute_neighbourhood_swap_fingerprint_diff(sol, mv, result);

    if (perform == NEIGHBOURHOOD_PERFORM_ALWAYS ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE && result->feasible) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_BETTER && result->delta_cost < 0) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER && result->feasible && result->delta_cost < 0)) {
        do_neighbourhood_swap(sol, mv->c1, mv->r1, mv->d1, mv->s1, mv->_c2, mv->r2, mv->d2, mv->s2);
        bool was_valid = solution_invalidate_helper(sol);
        if (result)
            result->_helper_was_valid = was_valid;
        return true;
    }

    return false;
}

void neighbourhood_swap_back(solution *sol,
                             const neighbourhood_swap_move *mv,
                             const neighbourhood_swap_result *res,
                             bool revalidate) {
    do_neighbourhood_swap(sol, mv->c1, mv->r2, mv->d2, mv->s2, mv->_c2, mv->r1, mv->d1, mv->s1);
    if (revalidate && res->_helper_was_valid && sol->helper)
        sol->helper->valid = true;
}

int neighbourhood_swap_move_cost(const neighbourhood_swap_move *move, solution *s) {
    return
        solution_curriculum_compactness_lecture_cost(s, move->c1, move->r1, move->d1, move->s1) * 10 +
        solution_curriculum_compactness_lecture_cost(s, move->_c2, move->r2, move->d2, move->s2) * 10;
    return
            solution_room_capacity_lecture_cost(s, move->c1, move->r1, move->d1, move->s1) +
            solution_min_working_days_course_cost(s, move->c1) +
            solution_curriculum_compactness_lecture_cost(s, move->c1, move->r1, move->d1, move->s1) * 10 +
            solution_room_stability_course_cost(s, move->c1) +
            solution_room_capacity_lecture_cost(s, move->_c2, move->r2, move->d2, move->s2) +
            solution_min_working_days_course_cost(s, move->_c2) +
            solution_curriculum_compactness_lecture_cost(s, move->_c2, move->r2, move->d2, move->s2) * 10 +
            solution_room_stability_course_cost(s, move->_c2);
}
