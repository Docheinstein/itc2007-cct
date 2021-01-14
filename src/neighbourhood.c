#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include "neighbourhood.h"
#include "utils/array_utils.h"
#include "solution.h"
#include "model.h"
#include "config.h"


void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, solution *sol) {
    iter->solution = sol;
    iter->end = false;
    iter->rds_index = -1;
    iter->c1 = iter->r1 = iter->d1 = iter->s1 = -1;
    iter->r2 = iter->d2 = iter->s2 = -1;
}

void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter) {}

bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, int *c1,
                                  int *r1, int *d1, int *s1,
                                  int *r2, int *d2, int *s2) {
    if (iter->end)
        return false;

    CRDSQT(iter->solution->model)
    const int RDS = R * D * S;
    const solution_helper *helper = solution_get_helper(iter->solution);

    iter->s2 = (iter->s2 + 1) % S;
    if (!iter->s2) {
        iter->d2 = (iter->d2 + 1) % D;
        if (!iter->d2) {
            iter->r2 = (iter->r2 + 1) % R;
            if (!iter->r2) {

                // Find next assignment
                do {
                    iter->rds_index++;
                } while ((helper->c_rds[iter->rds_index]) < 0 && iter->rds_index < RDS);

                if (iter->rds_index < RDS) {
                    iter->c1 = helper->c_rds[iter->rds_index];
                    iter->r1 = RINDEX3_0(iter->rds_index, R, D, S);
                    iter->d1 = RINDEX3_1(iter->rds_index, R, D, S);
                    iter->s1 = RINDEX3_2(iter->rds_index, R, D, S);
                    debug("Found next assignment (%d %d %d %d)",
                          iter->c1, iter->r1, iter->d1, iter->s1);
                } else {
                    debug("No assignment found, iter exhausted");
                    iter->end = true;
                    return false;
                }
            }
        }
    }

    *c1 = iter->c1;
    *r1 = iter->r1;
    *d1 = iter->d1;
    *s1 = iter->s1;
    *r2 = iter->r2;
    *d2 = iter->d2;
    *s2 = iter->s2;

    return true;
}


static bool check_neighbourhood_swap_hard_constraints(solution *sol,
                                                      int c1, int r1, int d1, int s1,
                                                      int c2, int r2, int d2, int s2) {

    CRDSQT(sol->model)
    const model *model = sol->model;
    const solution_helper *helper = solution_get_helper(sol);

    const bool same_period = d1 == d2 && s1 == s2;
    const bool same_room = r1 == r2;
    const bool same_course = c2 >= 0 && c1 == c2;
    const bool same_teacher = c2 >= 0 && model_same_teacher(model, c1, c2);

    debug2("c1 = %d -> %s | r1 = %d -> %s | d1 = %d | s1 = %d | "
           "c2 = %d -> %s | r2 = %d -> %s | d2 = %d | s2 = %d ",
           c1, c1 >= 0 ? model->courses[c1].id : "(none)", r1, model->rooms[r1].id, d1, s1,
           c2, c2 >= 0 ? model->courses[c2].id : "(none)", r2, model->rooms[r2].id, d2, s2);
    debug2("same_period = %d", same_period);
    debug2("same_course = %d", same_course);
    debug2("same_room = %d", same_room);
    debug2("same_teacher = %d", same_teacher);

    if (same_period && same_course && same_room)
        return true; // nothing to do

    // H1b
    if (c1 >= 0) {
        debug2("Check H1b (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (helper->sum_cds[INDEX3(c1, C, d2, D, s2, S)] - same_period - same_course > 0)
            return false;
    }

    if (c2 >= 0) {
        debug2("Check H1b (c=%d, d=%d, s=%d)", c2, d1, s1);
        if (helper->sum_cds[INDEX3(c2, C, d1, D, s1, S)] - same_period - same_course > 0)
            return false;
    }

    // H2
/*  NOT NEEDED?
    debug2("Check H2 (r=%d, d=%d, s=%d)", r2, d2, s2);
    if (helper->sum_rds[INDEX3(r2, R, d2, D, s2, S)] - same_period - same_room > 0)
        return false;

    if (c2 >= 0) {
        debug2("Check H2 (r=%d, d=%d, s=%d)", r1, d1, s1);
        if (helper->sum_rds[INDEX3(r1, R, d1, D, s1, S)] - same_period - same_room > 0)
            return false;
    }
*/
    // H3a

    if (c1 >= 0) {
        int c1_n_curriculas;
        int * c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
        for (int cq = 0; cq < c1_n_curriculas; cq++) {
            const int q = c1_curriculas[cq];
            bool share_curricula = c2 >= 0 && model_share_curricula(model, c1, c2, q);
            debug2("Check H3a (q=%d, d=%d, s=%d) -> %d", q, d2, s2, helper->sum_qds[INDEX3(q, Q, d2, D, s2, S)]);
            if (helper->sum_qds[INDEX3(q, Q, d2, D, s2, S)] - same_period - share_curricula > 0)
                return false;
        }
    }

    if (c2 >= 0) {
        int c2_n_curriculas;
        int * c2_curriculas = model_curriculas_of_course(model, c2, &c2_n_curriculas);
        for (int cq = 0; cq < c2_n_curriculas; cq++) {
            const int q = c2_curriculas[cq];
            bool share_curricula = c1 >= 0 && model_share_curricula(model, c1, c2, q);
            debug2("Check H3a (q=%d, d=%d, s=%d)", q, d1, s1);
            if (helper->sum_qds[INDEX3(q, Q, d1, D, s1, S)] - same_period - share_curricula > 0)
                return false;
        }
    }

    // H3b
    if (c1 >= 0) {
        const int t1 = model->courses[c1].teacher->index;
        debug2("Check H3b (t=%d, d=%d, s=%d)", t1, d2, s2);
        if (helper->sum_tds[INDEX3(t1, T, d2, D, s2, S)] - same_period - same_teacher > 0)
            return false;
    }

    if (c2 >= 0) {
        const int t2 = model->courses[c2].teacher->index;
        debug2("Check H3b (t=%d, d=%d, s=%d)", t2, d1, s1);
        if (helper->sum_tds[INDEX3(t2, T, d1, D, s1, S)] - same_period - same_teacher > 0)
            return false;
    }

    // H4
    if (c1 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (!model_course_is_available_on_period(model, c1, d2, s2))
            return false;
    }

    if (c2 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c2, d1, s1);
        if (!model_course_is_available_on_period(model, c2, d1, s1))
            return false;
    }


    return true;
}

static void do_neighbourhood_swap(
        solution *sol,
        int c1, int r1, int d1, int s1,
        int c2, int r2, int d2, int s2) {
    // Swap assignments
    debug("do_neighbourhood_swap(%d, %d, %d, %d <-> %d, %d, %d, %d)",
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

static bool do_neighbourhood_swap_if_possible(
        solution *sol,
        int c1, int r1, int d1, int s1,
        int c2, int r2, int d2, int s2) {
    bool satisfy_hard = check_neighbourhood_swap_hard_constraints(sol, c1, r1, d1, s1, c2, r2, d2, s2);
    if (!satisfy_hard)
        return false;

    do_neighbourhood_swap(sol, c1, r1, d1, s1, c2, r2, d2, s2);
    return true;
}

static int compute_neighbourhood_swap_room_capacity_cost(
        solution *sol, int c, int r_from, int r_to) {
    if (c < 0)
        return 0;

    int cost =
        MIN(0, sol->model->rooms[r_from].capacity - sol->model->courses[c].n_students) +
        MAX(0, sol->model->courses[c].n_students - sol->model->rooms[r_to].capacity);
    cost *= ROOM_CAPACITY_COST;

    debug("RoomCapacity delta cost: %d", cost);
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

    debug("MinWorkingDays delta cost: %d", cost);
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

    debug("RoomStability delta cost: %d", cost);
    return cost;
}

static int compute_neighbourhood_swap_curriculum_compactness_cost(
     solution *sol, int c_from, int d_from, int s_from, int c_to, int d_to, int s_to) {
    debug("CurriculumCompactness c=%d d=%d s=%d -> c=%d d=%d s=%d",
          c_from, d_from, s_from, c_to, d_to, s_to);
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
            debug("c=%d and c=%d share same curriculum q=%d, no swap cost", c_from, c_to, q);
            continue; // swap of courses of the same curriculum has no cost
        }

#define Z(q, d, s) \
    ((s) >= 0 && (s) < S && helper->sum_qds[INDEX3(q, Q, d, D, s, S)])

#define Z_OUT_AFTER(q, d, s) \
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
    (ALONE_OUT_BEFORE(q, d, s) && (!((d) == d_from && (s) == s_from)))

#define ALONE_IN_AFTER(q, d, s) \
    (Z_IN_AFTER(q, d, s) && !Z_IN_AFTER(q, d, (s) - 1) && !Z_IN_AFTER(q, d, (s) + 1))

        for (int s = 0; s < S; s++)
            debug("Z[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, Z(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("Z[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, Z(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("Z_OUT_AFTER[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, Z_OUT_AFTER(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("Z_OUT_AFTER[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, Z_OUT_AFTER(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("Z_IN_AFTER[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, Z_IN_AFTER(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("Z_IN_AFTER[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, Z_IN_AFTER(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_OUT_BEFORE[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, ALONE_OUT_BEFORE(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_OUT_BEFORE[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, ALONE_OUT_BEFORE(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_OUT_AFTER[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, ALONE_OUT_AFTER(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_OUT_AFTER[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, ALONE_OUT_AFTER(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_IN_BEFORE[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, ALONE_IN_BEFORE(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_IN_BEFORE[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, ALONE_IN_BEFORE(q, d_to, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_IN_AFTER[q=%d, d=d_from=%d, s=%d] = %d", q, d_from, s, ALONE_IN_AFTER(q, d_from, s));
        debug("----");

        for (int s = 0; s < S; s++)
            debug("ALONE_IN_AFTER[q=%d, d=d_to=%d, s=%d] = %d", q, d_to, s, ALONE_IN_AFTER(q, d_to, s));
        debug("----");

        int out_prev_cost_before = ALONE_OUT_BEFORE(q, d_from, s_from - 1);
        int out_itself_cost = ALONE_OUT_BEFORE(q, d_from, s_from);
        int out_next_cost_before = ALONE_OUT_BEFORE(q, d_from, s_from + 1);

        int out_prev_cost_after = ALONE_OUT_AFTER(q, d_from, s_from - 1);
        int out_next_cost_after = ALONE_OUT_AFTER(q, d_from, s_from + 1);

        int in_prev_cost_before = ALONE_OUT_AFTER(q, d_to, s_to - 1);
        int in_next_cost_before = ALONE_OUT_AFTER(q, d_to, s_to + 1);
        int in_prev_cost_after = ALONE_IN_AFTER(q, d_to, s_to - 1);
        int in_next_cost_after = ALONE_IN_AFTER(q, d_to, s_to + 1);

        int in_itself_cost = ALONE_IN_AFTER(q, d_to, s_to);

        debug("[q=%d] out_prev_cost_before = %d", q, out_prev_cost_before);
        debug("[q=%d] out_itself_cost = %d", q, out_itself_cost);
        debug("[q=%d] out_next_cost_before = %d", q, out_next_cost_before);

        debug("[q=%d] out_prev_cost_after = %d", q, out_prev_cost_after);
        debug("[q=%d] out_next_cost_after = %d", q, out_next_cost_after);

        debug("[q=%d] in_prev_cost_before = %d", q, in_prev_cost_before);
        debug("[q=%d] in_next_cost_before = %d", q, in_next_cost_before);
        debug("[q=%d] in_prev_cost_after = %d", q, in_prev_cost_after);
        debug("[q=%d] in_next_cost_after = %d", q, in_next_cost_after);

        debug("[q=%d] in_itself_cost = %d", q, in_itself_cost);

        int q_cost =
            (out_prev_cost_after - out_prev_cost_before) +
            (out_next_cost_after - out_next_cost_before) +
            (in_prev_cost_after - in_prev_cost_before) +
            (in_next_cost_after - in_next_cost_before) +
            (in_itself_cost - out_itself_cost);

        debug("[q=%d] cost = %d", q, q_cost);

#undef Z
#undef ALONE_OUT_BEFORE

        cost += q_cost;
    }

    cost *= CURRICULUM_COMPACTNESS_COST;

    debug("CurriculumCompactness delta cost: %d", cost);

    return cost;
}

static void compute_neighbourhood_swap_cost(
        solution *sol,
        int c1, int r1, int d1, int s1,
        int c2, int r2, int d2, int s2,
        neighbourhood_swap_result *result) {
    CRDSQT(sol->model)

    result->delta_cost_room_capacity = 0;
    result->delta_cost_min_working_days = 0;
    result->delta_cost_curriculum_compactness = 0;
    result->delta_cost_room_stability = 0;

    const solution_helper *helper = solution_get_helper(sol);
    const model *model = sol->model;

    // Room capacity
    result->delta_cost_room_capacity +=
        compute_neighbourhood_swap_room_capacity_cost(sol, c1, r1, r2) +
        compute_neighbourhood_swap_room_capacity_cost(sol, c2, r2, r1);

    // MinWorkingDays
    result->delta_cost_min_working_days +=
        compute_neighbourhood_swap_min_working_days_cost(sol, c1, d1, c2, d2) +
        compute_neighbourhood_swap_min_working_days_cost(sol, c2, d2, c1, d1);

    // CurriculumCompactness
    result->delta_cost_curriculum_compactness +=
        compute_neighbourhood_swap_curriculum_compactness_cost(sol, c1, d1, s1, c2, d2, s2) +
        compute_neighbourhood_swap_curriculum_compactness_cost(sol, c2, d2, s2, c1, d1, s1);

    // RoomStability
    result->delta_cost_room_stability +=
        compute_neighbourhood_swap_room_stability_cost(sol, c1, r1, c2, r2) +
        compute_neighbourhood_swap_room_stability_cost(sol, c2, r2, c1, r1);

    result->delta_cost =
            result->delta_cost_room_capacity +
            result->delta_cost_min_working_days +
            result->delta_cost_curriculum_compactness +
            result->delta_cost_room_stability;

    debug("DELTA cost = %d", result->delta_cost);
}

void neighbourhood_swap(solution *sol, int c1,
                        int r1, int d1, int s1,
                        int r2, int d2, int s2,
                        neighbourhood_swap_result *result) {
    CRDSQT(sol->model)

    int c2 = solution_get_helper(sol)->c_rds[INDEX3(r2, R, d2, D, s2, S)];
    result->c2 = c2;

#if DEBUG
    if (!solution_get(sol, c1, r1, d1, s1)) {
        eprint("Expected true in timetable (%d %d %d %d) -> %d",
               c1, r1, d1, s1, INDEX4(c1, C, r1, R, d1, D, s1, S));
        print_solution(sol, stderr);
        exit(0);
    }
#endif

    result->feasible = do_neighbourhood_swap_if_possible(sol, c1, r1, d1, s1, c2, r2, d2, s2);
    if (result->feasible)
        compute_neighbourhood_swap_cost(sol, c1, r1, d1, s1, c2, r2, d2, s2, result);
}

void neighbourhood_swap_course_lectures_room_iter_init(
        neighbourhood_swap_course_lectures_room_iter *iter,
        const solution *sol) {
    iter->solution = sol;
    iter->room_index = iter->course_index = 0;
}

void neighbourhood_swap_course_lectures_room_iter_destroy(
        neighbourhood_swap_course_lectures_room_iter *iter) {

}

bool neighbourhood_swap_course_lectures_room_iter_next(
        neighbourhood_swap_course_lectures_room_iter *iter, int *c, int *r) {
    *c = iter->course_index;
    *r = iter->room_index;

    iter->room_index = (iter->room_index + 1) % (iter->solution->model->n_rooms);
    iter->course_index += iter->room_index == 0 ? 1 : 0;

    return iter->course_index < iter->solution->model->n_courses;
}

bool neighbourhood_swap_course_lectures_room(
        solution *sol, int c, int r) {
    CRDSQT(sol->model)

    return true;
//    for (int d = 0; d < D; d++) {
//        for (int s = 0; s < S; s++) {
//            int c_room = sol_in->helper.r_cds[INDEX3(c, C, d, D, s, S)];
//            int r_course = sol_in->helper.c_rds[INDEX3(r, R, d, D, s, S)];
//
//            do_neighbourhood_swap(sol_out)
//        }
//    }
//    return 0;
}

void neighbourhood_swap_back(solution *sol,
                             int c1, int r1, int d1, int s1,
                             int c2, int r2, int d2, int s2) {
    do_neighbourhood_swap(sol, c1, r2, d2, s2, c2, r1, d1, s1);
}
