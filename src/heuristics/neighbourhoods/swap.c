#include "swap.h"
#include "log/debug.h"
#include "utils/io_utils.h"
#include "utils/rand_utils.h"
#include "utils/array_utils.h"
#include "utils/assert_utils.h"
#include "solution/solution.h"
#include "model/model.h"


/*
 * Returns 'true' if course c1 can be assigned
 * - from (day=d2, slot=s1)
 * - to (day=d2, slot=s2) (previously occupied by c2)
 * without breaking 'Lectures' constraint.
 *
 * [All lectures of a course must be assigned to distinct periods]
 */
static bool check_lectures_constraint(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    MODEL(sol->model);
    const bool same_period = d1 == d2 && s1 == s2;
    const bool same_course = c1 == c2;

    if (c1 >= 0) {
        debug2("Check H1b (Lectures) (c=%d, d=%d, s=%d) -> %d", c1, d2, s2, sol->sum_cds[INDEX3(c1, C, d2, D, s2, S)]);
        if (sol->sum_cds[INDEX3(c1, C, d2, D, s2, S)] - same_period - same_course > 0)
            return false;
    }

    return true;
}

/*
 * Returns 'true' if course c1 can be assigned
 * - from (day=d2, slot=s1)
 * - to (day=d2, slot=s2) (previously occupied by c2)
 * without breaking 'Curriculum Conflicts' constraint.
 *
 * [Lectures of courses in the same curriculum must be all scheduled in different periods]
 */
static bool check_conflicts_curriculum_constraint(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    MODEL(sol->model);
    const bool same_period = d1 == d2 && s1 == s2;

    if (c1 >= 0) {
        int c1_n_curriculas;
        int *c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
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

/*
 * Returns 'true' if course c1 can be assigned
 * - from (day=d2, slot=s1)
 * - to (day=d2, slot=s2) (previously occupied by c2)
 * without breaking 'Teacher Conflicts' constraint.
 *
 * [Lectures of courses taught by the same teacher must be all scheduled in different periods]
 */
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

/*
 * Returns 'true' if course c1 can be assigned
 * - to (day=d2, slot=s2)
 * without breaking 'Availabilities' constraint.
 *
 * [If the teacher of the course is not available to teach that course at a given period,
 *  then no lectures of the course can be scheduled at that period]
 */
static bool check_availabilities_constraint(
        const solution *sol, int c1, int d2, int s2) {
    if (c1 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c1, d2, s2);
        if (!model_course_is_available_on_period(sol->model, c1, d2, s2))
            return false;
    }

    return true;
}

/*
 * Compute 'RoomCapacity' cost of assigning c1
 * - to (day=d2, slot=s2)
 *
 * [For each lecture, the number of students that attend the course must be
 *  less or equal than the number of seats of all the rooms that host its lectures.
 *  Each student above the capacity counts as 1 point of penalty]
 */
static int compute_room_capacity_cost(
        const solution *sol, int c1, int r1, int r2) {
    if (c1 < 0)
        return 0;

    int cost =
        MIN(0, sol->model->rooms[r1].capacity - sol->model->courses[c1].n_students) +
        MAX(0, sol->model->courses[c1].n_students - sol->model->rooms[r2].capacity);
    cost *= ROOM_CAPACITY_COST_FACTOR;

    debug2("RoomCapacity delta cost: %d", cost);
    return cost;
}

/*
 * Compute 'MinimumWorkingDays' cost of assigning c1
 * - from day d2
 * - to day d2 (previously occupied by c2)
 *
 * [The lectures of each course must be spread into the given mini-
 *  mum number of days. Each day below the minimum counts as 5 points of penalty]
 */
static int compute_min_working_days_cost(
        const solution *sol, int c1, int d1, int c2, int d2) {

    if (c1 < 0)
        return 0;
    if (c1 == c2)
        return 0;

    MODEL(sol->model);

    int min_working_days = sol->model->courses[c1].min_working_days;
    int prev_working_days = 0;
    int cur_working_days = 0;
    FOR_D {
        prev_working_days += MIN(1, sol->sum_cd[INDEX2(c1, C, d, D)]);
        cur_working_days += MIN(1, sol->sum_cd[INDEX2(c1, C, d, D)] - (d == d1) + (d == d2));
    }

    int cost =
            MIN(0, prev_working_days - min_working_days) +
            MAX(0, min_working_days - cur_working_days);
    cost *= MIN_WORKING_DAYS_COST_FACTOR;

    debug2("MinWorkingDays delta cost: %d", cost);
    return cost;
}

/*
 * Compute 'RoomStability' cost of assigning c1
 * - from room r1
 * - to room r2 (previously occupied by c2)
 *
 * [All lectures of a course should be given in the same room. Each distinct
 *  room used for the lectures of a course, but the first, counts as 1 point of penalty]
 */
static int compute_room_stability_cost(
        const solution *sol, int c1, int r1, int c2, int r2) {
    if (c1 < 0)
        return 0;
    if (r1 == r2)
        return 0;
    if (c1 == c2)
        return 0;

    MODEL(sol->model);

    int prev_rooms = 0;
    int cur_rooms = 0;
    FOR_R {
        prev_rooms += MIN(1, sol->sum_cr[INDEX2(c1, C, r, R)]);
        cur_rooms += MIN(1, sol->sum_cr[INDEX2(c1, C, r, R)] - (r == r1) + (r == r2));
    };

    int cost = MAX(0, cur_rooms - 1) - MAX(0, prev_rooms - 1);
    cost *= ROOM_STABILITY_COST_FACTOR;

    debug2("RoomStability delta cost: %d", cost);
    return cost;
}

/*
 * Compute 'CurriculumCompactness' cost of assigning c1
 * - from (day=d1, slot=s1)
 * - to (day=d2, slot=s2) (previously occupied by c2)
 *
 * [Lectures belonging to a curriculum should be adjacent to each
 *  other (i.e., in consecutive periods). For a given curriculum we account for a violation
 *  every time there is one lecture not adjacent to any other lecture within the same day.
 *  Each isolated lecture in a curriculum counts as 2 points of penalty]
 */
static int compute_curriculum_compactness_cost(
        const solution *sol, int c1, int d1, int s1, int c2, int d2, int s2) {
    if (c1 < 0)
        return 0;
    if (c1 == c2)
        return 0;

    MODEL(sol->model);

#define QDS(q, d, s) \
    ((s) >= 0 && (s) < S && sol->sum_qds[INDEX3(q, Q, d, D, s, S)])

#define QDS_OUT_AFTER(q, d, s) \
    (!((d) == d1 && (s) == s1) && \
    ((QDS(q, d, s))))

#define QDS_IN_BEFORE(q, d, s) \
    (!((d) == d1 && (s) == s1) && \
    ((QDS(q, d, s))))

#define QDS_IN_AFTER(q, d, s) \
    (((s) == s2 && (d) == d2) || \
    (!((d) == d1 && (s) == s1) && \
    ((QDS(q, d, s)))))

#define ALONE_OUT_BEFORE(q, d, s) \
    (QDS(q, d, s) && !QDS(q, d, (s) - 1) && !QDS(q, d, (s) + 1))

#define ALONE_OUT_AFTER(q, d, s) \
    (QDS_OUT_AFTER(q, d, s) && !QDS_OUT_AFTER(q, d, (s) - 1) && !QDS_OUT_AFTER(q, d, (s) + 1))

#define ALONE_IN_BEFORE(q, d, s) \
    (QDS_IN_BEFORE(q, d, s) && !QDS_IN_BEFORE(q, d, (s) - 1) && !QDS_IN_BEFORE(q, d, (s) + 1))

#define ALONE_IN_AFTER(q, d, s) \
    (QDS_IN_AFTER(q, d, s) && !QDS_IN_AFTER(q, d, (s) - 1) && !QDS_IN_AFTER(q, d, (s) + 1))
    
    
    int cost = 0;

    int c_n_curriculas;
    int *c_curriculas = model_curriculas_of_course(model, c1, &c_n_curriculas);

    /*
     * The logic for compute the compactness cost is the following:
     * for each curricula the course c1 belongs to:
     * - if c2 share a curriculum with c1, no cost is introduced
     * - otherwise:
     *      the cost is given by summing the cost of taking c1 out
     *      of (d1, s1), minus the bonus of placing c1 into (d2, s2);
     *      the costs depend on whether the curriculum has any adjacent
     *      lecture or not.
     *  It might seems a little bit messy: it is.
     *  By the way, what follows works, if the current solution if feasible.
     */
    for (int cq = 0; cq < c_n_curriculas; cq++) {
        int q = c_curriculas[cq];

        if (c2 >= 0 && model_share_curricula(model, c2, c1, q))
            continue; // swap of courses of the same curriculum has no cost

        int out_prev_cost_before = ALONE_OUT_BEFORE(q, d1, s1 - 1);
        int out_itself_cost = ALONE_OUT_BEFORE(q, d1, s1);
        int out_next_cost_before = ALONE_OUT_BEFORE(q, d1, s1 + 1);

        int out_prev_cost_after = ALONE_OUT_AFTER(q, d1, s1 - 1);
        int out_next_cost_after = ALONE_OUT_AFTER(q, d1, s1 + 1);

        int in_prev_cost_before = ALONE_IN_BEFORE(q, d2, s2 - 1);
        int in_next_cost_before = ALONE_IN_BEFORE(q, d2, s2 + 1);
        int in_prev_cost_after = ALONE_IN_AFTER(q, d2, s2 - 1);
        int in_next_cost_after = ALONE_IN_AFTER(q, d2, s2 + 1);

        int in_itself_cost = ALONE_IN_AFTER(q, d2, s2);

        int q_cost =
            (out_prev_cost_after - out_prev_cost_before) +
            (out_next_cost_after - out_next_cost_before) +
            (in_prev_cost_after - in_prev_cost_before) +
            (in_next_cost_after - in_next_cost_before) +
            (in_itself_cost - out_itself_cost);

        debug2("compute_curriculum_compactness_cost q=%d:%s\n"
                "   c1=%d, d1=%d, s1=%d, c2=%d, d2=%d, s2=%d\n"
                "   (out_prev_cost_after %d   -  out_prev_cost_before %d) +\n"
                "   (out_next_cost_after %d   -  out_next_cost_before %d) +\n"
                "   (in_prev_cost_after  %d   -  in_prev_cost_before  %d) +\n"
                "   (in_next_cost_after  %d   -  out_prev_cost_before %d) +\n"
                "   (in_itself_cost      %d   -  out_itself_cost      %d) = %d",
               q, model->curriculas[q].id,
               c1, d1, s1, c2, d2, s2,
               out_prev_cost_after, out_prev_cost_before,
               out_next_cost_after, out_next_cost_before,
               in_prev_cost_after, in_prev_cost_before,
               in_next_cost_after, in_next_cost_before,
               in_itself_cost, out_itself_cost,
               q_cost
        );
        cost += q_cost;
    }

    cost *= CURRICULUM_COMPACTNESS_COST_FACTOR;

    debug2("CurriculumCompactness delta cost: %d", cost);

#undef QDS
#undef QDS_OUT_AFTER
#undef QDS_IN_BEFORE
#undef QDS_IN_AFTER
#undef ALONE_OUT_BEFORE
#undef ALONE_OUT_AFTER
#undef ALONE_IN_BEFORE
#undef ALONE_IN_AFTER

    return cost;
}


static bool swap_move_check_hard_constraints(const solution *sol,
                                             const swap_move *mv) {
    debug2("Checking hard constraints of (%d, %d, %d, %d) <-> (%d, %d, %d, %d) // (%s, %s, %d, %d) <-> (%s, %s, %d, %d)",
           mv->helper.c1, mv->helper.r1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->r2, mv->d2, mv->s2,
           mv->helper.c1 >= 0 ? sol->model->courses[mv->helper.c1].id : "-",
           mv->helper.r1 >= 0 ? sol->model->rooms[mv->helper.r1].id : "-",
           mv->helper.d1, mv->helper.s1,
           mv->helper.c2 >= 0 ? sol->model->courses[mv->helper.c2].id : "-",
           mv->r2 >= 0 ? sol->model->rooms[mv->r2].id : "-",
           mv->d2, mv->s2
           );

    if (mv->helper.c1 == mv->helper.c2)
        return true; // swap two lectures of same course is always legal

    // Lectures
    if (!check_lectures_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;
    if (!check_lectures_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;

    // RoomOccupancy: no need to check since the swap replaces the room by design

    // Availabilities
    if (!check_availabilities_constraint(
            sol, mv->helper.c1, mv->d2, mv->s2))
        return false;

    if (!check_availabilities_constraint(
            sol, mv->helper.c2, mv->helper.d1, mv->helper.s1))
        return false;

    // Conflicts: teacher
    if (!check_conflicts_teacher_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;

    if (!check_conflicts_teacher_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;

    // Conflicts: curriculum (as last, since is the toughest to compute)
    if (!check_conflicts_curriculum_constraint(
            sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2))
        return false;

    if (!check_conflicts_curriculum_constraint(
            sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1))
        return false;
    return true;
}

static void swap_move_compute_cost(
        const solution *sol,
        const swap_move *mv,
        swap_result *result) {
    MODEL(sol->model);
    static int non_gave_up = 0;
    static int gave_up = 0;

    result->delta.room_capacity_cost = 0;
    result->delta.min_working_days_cost = 0;
    result->delta.curriculum_compactness_cost = 0;
    result->delta.room_stability_cost = 0;

    // Room capacity
    result->delta.room_capacity_cost +=
            compute_room_capacity_cost(sol, mv->helper.c1, mv->helper.r1, mv->r2);
    result->delta.room_capacity_cost +=
            compute_room_capacity_cost(sol, mv->helper.c2, mv->r2, mv->helper.r1);

    // MinWorkingDays
    result->delta.min_working_days_cost +=
            compute_min_working_days_cost(sol, mv->helper.c1, mv->helper.d1, mv->helper.c2, mv->d2);
    result->delta.min_working_days_cost +=
            compute_min_working_days_cost(sol, mv->helper.c2, mv->d2, mv->helper.c1, mv->helper.d1);

    // CurriculumCompactness
    result->delta.curriculum_compactness_cost +=
            compute_curriculum_compactness_cost(
                    sol, mv->helper.c1, mv->helper.d1, mv->helper.s1, mv->helper.c2, mv->d2, mv->s2);
    result->delta.curriculum_compactness_cost +=
            compute_curriculum_compactness_cost(
                    sol, mv->helper.c2, mv->d2, mv->s2, mv->helper.c1, mv->helper.d1, mv->helper.s1);

    // RoomStability
    result->delta.min_working_days_cost +=
            compute_room_stability_cost(sol, mv->helper.c1, mv->helper.r1, mv->helper.c2, mv->r2);
    result->delta.min_working_days_cost +=
            compute_room_stability_cost(sol, mv->helper.c2, mv->r2, mv->helper.c1, mv->helper.r1);

    result->delta.cost =
            result->delta.room_capacity_cost +
            result->delta.min_working_days_cost +
            result->delta.curriculum_compactness_cost +
            result->delta.room_stability_cost;
    debug2("swap_move_compute_cost cost = %d", result->delta.cost);
}

static void swap_move_do(solution *sol, const swap_move *mv) {
//    print("mv: %d -> %d %d %d", mv->l1, mv->r2, mv->d2, mv->s2);
    assert(sol->assignments[mv->l1].r == mv->helper.r1);
    assert(sol->assignments[mv->l1].d == mv->helper.d1);
    assert(sol->assignments[mv->l1].s == mv->helper.s1);

    solution_unassign_lecture(sol, mv->l1);
    if (mv->helper.l2 >= 0)
        solution_unassign_lecture(sol, mv->helper.l2);
    solution_assign_lecture(sol, mv->l1, mv->r2, mv->d2, mv->s2);
    if (mv->helper.l2 >= 0)
        solution_assign_lecture(sol, mv->helper.l2, mv->helper.r1, mv->helper.d1, mv->helper.s1);
}

void swap_iter_init(swap_iter *iter, const solution *sol) {
    iter->solution = sol;
    iter->move.l1 = iter->move.r2 = iter->move.d2 = iter->move.s2 = -1;
    iter->end = false;
    iter->i = 0;
}

void swap_iter_destroy(swap_iter *iter) {}

bool swap_move_is_effective(const swap_move *mv) {
    return mv->helper.c1 != mv->helper.c2; // swap of the same course is not effective
}

/*
 * Compute the swap_move's data that depends on the current solution.
 * (Will be valid until the solution is touched).
 */
void swap_move_compute_helper(const solution *sol, swap_move *mv) {
    MODEL(sol->model);

    mv->helper.c1 = sol->model->lectures[mv->l1].course->index;

    const assignment *a = &sol->assignments[mv->l1];
    mv->helper.r1 = a->r;
    mv->helper.d1 = a->d;
    mv->helper.s1 = a->s;

    mv->helper.l2 = sol->l_rds[INDEX3(mv->r2, R, mv->d2, D, mv->s2, S)];
    if (mv->helper.l2 >= 0)
        mv->helper.c2 = sol->model->lectures[mv->helper.l2].course->index;
    else
        mv->helper.c2 = -1;

    debug2("swap_move_compute_helper: "
           "COMPUTED (l=%d, c=%d, r=%d, d=%d, s=%d) <-> (l=%d, c=%d, r=%d, d=%d, s=%d)",
           mv->l1, mv->helper.c1, mv->helper.r1, mv->helper.d1, mv->helper.s1,
           mv->helper.l2, mv->helper.c2, mv->r2, mv->d2, mv->s2);
}

void swap_move_copy(swap_move *dest, const swap_move *src) {
    memcpy(dest, src, sizeof(swap_move));
}

void swap_move_generate_random_raw(const solution *sol, swap_move *mv) {
    MODEL(sol->model);
    mv->l1 = rand_range(0, L);
    mv->r2 = rand_range(0, R);
    mv->d2 = rand_range(0, D);
    mv->s2 = rand_range(0, S);
    swap_move_compute_helper(sol, mv);
}

void swap_move_generate_random_extended(const solution *sol, swap_move *mv,
                                        bool require_effectiveness, bool require_feasibility) {
    MODEL(sol->model);
    swap_result result = {.feasible = false};

    do {
        do {
            swap_move_generate_random_raw(sol, mv);
        } while (require_effectiveness && !swap_move_is_effective(mv));

        if (require_feasibility)
            swap_predict(sol, mv,
                         NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                         NEIGHBOURHOOD_PREDICT_COST_NEVER,
                         &result);
    } while(require_feasibility && !result.feasible);
}

void swap_move_generate_random_feasible_effective(const solution *sol, swap_move *mv) {
    swap_move_generate_random_extended(sol, mv, true, true);
}

void swap_move_reverse(const swap_move *mv, swap_move *reverse_mv) {
    reverse_mv->l1 = mv->l1;
    reverse_mv->helper.c1 = mv->helper.c1;
    reverse_mv->helper.l2 = mv->helper.l2;
    reverse_mv->helper.c2 = mv->helper.c2;

    reverse_mv->r2 = mv->helper.r1;
    reverse_mv->d2 = mv->helper.d1;
    reverse_mv->s2 = mv->helper.s1;
    reverse_mv->helper.r1 = mv->r2;
    reverse_mv->helper.d1 = mv->d2;
    reverse_mv->helper.s1 = mv->s2;
}


bool swap_iter_next(swap_iter *iter) {
    if (iter->end)
        return false;

    MODEL(iter->solution->model);

    do {
        iter->move.s2 = (iter->move.s2 + 1) % S;
        if (!iter->move.s2) {
            iter->move.d2 = (iter->move.d2 + 1) % D;
            if (!iter->move.d2) {
                iter->move.r2 = (iter->move.r2 + 1) % R;
                if (!iter->move.r2) {
                    iter->move.l1 = (iter->move.l1 + 1);
                    if (!(iter->move.l1 < L)) {
                        debug2("Iter exhausted after %d iters", iter->i);
                        iter->end = true;
                        return false;
                    }
                }
            }
        }
        swap_move_compute_helper(iter->solution, &iter->move);
    } while (iter->move.helper.c1 <= iter->move.helper.c2);

    assert(swap_move_is_effective(&iter->move));

    debug2("swap_iter_next: l=%d -> (r=%d, d=%d, s=%d)",
           iter->move.l1, iter->move.r2, iter->move.d2, iter->move.s2);

    iter->i++;

    return true;
}


void swap_predict(const solution *sol, const swap_move *move,
                  neighbourhood_predict_feasibility_strategy predict_feasibility,
                  neighbourhood_predict_cost_strategy predict_cost,
                  swap_result *result) {
    if (predict_feasibility == NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS)
        if (result)
            result->feasible = swap_move_check_hard_constraints(sol, move);

    if (predict_cost == NEIGHBOURHOOD_PREDICT_COST_ALWAYS ||
        (predict_cost == NEIGHBOURHOOD_PREDICT_COST_IF_FEASIBLE && result->feasible))
        swap_move_compute_cost(sol, move, result);
}

bool swap_perform(solution *sol, const swap_move *move,
                  neighbourhood_perform_strategy perform,
                  swap_result *result) {
    if (perform == NEIGHBOURHOOD_PERFORM_ALWAYS ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE && result->feasible) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_BETTER && result->delta.cost < 0) ||
            (perform == NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER &&
                result->feasible && result->delta.cost < 0)) {
        swap_move_do(sol, move);
        return true;
    }
    return false;
}

//bool swap_extended(solution *sol, const swap_move *mv,
//                   neighbourhood_predict_feasibility_strategy predict_feasibility,
//                   neighbourhood_predict_cost_strategy predict_cost,
//                   neighbourhood_perform_strategy perform,
//                   swap_result *result) {
//    swap_predict(sol, mv, predict_feasibility, predict_cost, result);
//    return swap_perform(sol, mv, perform, result);
//}
