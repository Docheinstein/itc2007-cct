#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include "neighbourhood.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "solution.h"
#include "model.h"

#if 0
void neighbourhood_swap_assignment(solution *sol) {
#if DEBUG
    unsigned long long fingerprint = solution_fingerprint(sol);
#endif
    const int C = sol->model->n_courses;
    const int R = sol->model->n_rooms;
    const int D = sol->model->n_days;
    const int S = sol->model->n_slots;
    size_t ttsize = C * R * D * S;

    // Find the assignments in the timetable
    int * assignment_indexes = mallocx(ttsize, sizeof(int));
    int cursor = 0;
    for (int i = 0; i < ttsize; i++) {
        if (solution_get_at(sol, i))
            assignment_indexes[cursor++] = i;
    }

    debug("There are %d/%lu", cursor, ttsize);

    if (!cursor)
        return; // nothing to do, no assignments in timetable

    // Pick an assignment at random
    int assignment_index = assignment_indexes[rand_int_range(0, cursor)];
    debug("Random assignment_index = %d", assignment_index);
    int a_c = RINDEX4_0(assignment_index, C, R, D, S);
    int a_r = RINDEX4_1(assignment_index, C, R, D, S);
    int a_d = RINDEX4_2(assignment_index, C, R, D, S);
    int a_s = RINDEX4_3(assignment_index, C, R, D, S);

#if DEBUG
    if (!solution_get(sol, a_c, a_r, a_d, a_s)) {
        eprint("Expected 1 in timetable");
        exit(0);
    }
#endif

    // Pick a (room,day,slot) at random
    int r_r, r_d, r_s;
    do {  // avoid to swap two lectures of the same course
        r_r = rand_int_range(0, sol->model->n_rooms);
        r_d = rand_int_range(0, sol->model->n_days);
        r_s = rand_int_range(0, sol->model->n_slots);
    } while (solution_get(sol, a_c, r_r, r_d, r_s));

    debug("Random (room,day,slot) = (%s, %d, %d)",
          sol->model->rooms[r_r].id, r_d, r_s);

    int c_star = -1;
    // Check if there is already a course in the picked (room,day,slot)
    for (int c = 0; c < C; c++) {
        if (solution_get(sol, c, r_r, r_d, r_s)) {
            c_star = c;
            break;
        }
    }

    // Swap assignments
    solution_set_at(sol, assignment_index, false);
    solution_set(sol, a_c, r_r, r_d, r_s, true);

    if (c_star >= 0) {
        debug("Found course %s scheduled in slot (%s, %d, %d)",
                sol->model->courses[c_star].id, sol->model->rooms[r_r].id, r_d, r_s);
        solution_set(sol, c_star, r_r, r_d, r_s, false);
        solution_set(sol, c_star, a_r, a_d, a_s, true);
    } else {
        debug("No course found in slot (%s, %d, %d)",
              sol->model->rooms[r_r].id, r_d, r_s);
    }

    if (!solution_satisfy_hard_constraints(sol)) {
        debug("Swap breaks hard constraints, reverting changes");

        solution_set_at(sol, assignment_index, true);
        solution_set(sol, a_c, r_r, r_d, r_s, false);

        if (c_star >= 0) {
            solution_set(sol, c_star, r_r, r_d, r_s, true);
            solution_set(sol, c_star, a_r, a_d, a_s, false);
        }

#ifdef DEBUG
        if (solution_fingerprint(sol) != fingerprint) {
            eprint("Fingeprint should be the same after reverting");
            exit(2);
        }
#endif
    }
}
#endif


void neighbourhood_swap_iter_init(neighbourhood_swap_iter *iter, const solution *sol) {
    CRDSQT(sol->model)

    size_t ttsize = C * R * D * S;

    iter->solution = sol;

    iter->assignments = mallocx(ttsize, sizeof(int));
    // Find the assignments in the timetable
    int cursor = 0;
    for (int i = 0; i < ttsize; i++) {
        if (solution_get_at(sol, i)) {
            debug("Found assignment[%d]=%d", cursor, i);
            iter->assignments[cursor++] = i;
        }
    }

    iter->n_assignments = cursor;
    debug2("There are %d/%lu assignments", iter->n_assignments, ttsize);

    if (cursor > 0) {
        iter->assignment_index = iter->roomdayslot_index = 0;
    } else {
        iter->assignment_index = iter->roomdayslot_index = -1;
    }
}

void neighbourhood_swap_iter_destroy(neighbourhood_swap_iter *iter) {
    free(iter->assignments);
}

bool neighbourhood_swap_iter_next(neighbourhood_swap_iter *iter, int *c1,
                                  int *r1, int *d1, int *s1,
                                  int *r2, int *d2, int *s2) {
    if (iter->assignment_index < 0)
        return false;

    CRDSQT(iter->solution->model)

    *c1 = RINDEX4_0(iter->assignments[iter->assignment_index], C, R, D, S);
    *r1 = RINDEX4_1(iter->assignments[iter->assignment_index], C, R, D, S);
    *d1 = RINDEX4_2(iter->assignments[iter->assignment_index], C, R, D, S);
    *s1 = RINDEX4_3(iter->assignments[iter->assignment_index], C, R, D, S);
    *r2 = RINDEX3_0(iter->roomdayslot_index, R, D, S);
    *d2 = RINDEX3_1(iter->roomdayslot_index, R, D, S);
    *s2 = RINDEX3_2(iter->roomdayslot_index, R, D, S);

    iter->roomdayslot_index = (iter->roomdayslot_index + 1) % (R * D * S);
    iter->assignment_index += iter->roomdayslot_index == 0 ? 1 : 0;

    return iter->assignment_index < iter->n_assignments;
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
        const int t1 = model_teacher_by_id(model, model->courses[c1].teacher_id)->index;
        debug2("Check H3b (t=%d, d=%d, s=%d)", t1, d2, s2);
        if (helper->sum_tds[INDEX3(t1, T, d2, D, s2, S)] - same_period - same_teacher > 0)
            return false;
    }

    if (c2 >= 0) {
        const int t2 = model_teacher_by_id(model, model->courses[c2].teacher_id)->index;
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

bool neighbourhood_swap(solution *sol, int c1,
                        int r1, int d1, int s1,
                        int r2, int d2, int s2,
                        int *c2) {
    CRDSQT(sol->model)

    int c2_ = solution_get_helper(sol)->c_rds[INDEX3(r2, R, d2, D, s2, S)];
    if (c2)
        *c2 = c2_;

#if DEBUG
    if (!solution_get(sol, c1, r1, d1, s1)) {
        eprint("Expected true in timetable");
        exit(0);
    }
#endif

    return do_neighbourhood_swap_if_possible(sol, c1, r1, d1, s1, c2_, r2, d2, s2);
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
