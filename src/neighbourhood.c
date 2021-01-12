#include <log/debug.h>
#include <utils/io_utils.h>
#include <utils/str_utils.h>
#include "neighbourhood.h"
#include "utils/mem_utils.h"
#include "utils/random_utils.h"
#include "utils/array_utils.h"
#include "solution.h"
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


#define CRDSQT \
    const int C = model->n_courses; \
    const int R = model->n_rooms;   \
    const int D = model->n_days;   \
    const int S = model->n_slots;   \
    const int T = model->n_teachers; \
    const int Q = model->n_curriculas; \

void neighbourhood_iter_init(neighbourhood_iter *iter, const solution *sol) {
    const model *model = sol->model;
    CRDSQT

    size_t ttsize = C * R * D * S;

    iter->solution = sol;

    iter->assignments = mallocx(ttsize, sizeof(int));
    // Find the assignments in the timetable
    int cursor = 0;
    for (int i = 0; i < ttsize; i++) {
        if (solution_get_at(sol, i)) {
            debug("[%d] Found assignment at index %d", cursor, i);
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

void neighbourhood_iter_destroy(neighbourhood_iter *iter) {
    free(iter->assignments);
}

bool neighbourhood_iter_next(neighbourhood_iter *iter, int *c1,
                             int *r1, int *d1, int *s1,
                             int *r2, int *d2, int *s2) {
    if (iter->assignment_index < 0)
        return false;

    const model *model = iter->solution->model;
    CRDSQT

    debug2("neighbourhood_iter_next for iter->assignment_index = %d | iter->assignments[iter->assignment_index] = %d",
           iter->assignment_index, iter->assignments[iter->assignment_index]);
    debug2(" = %d", iter->solution->timetable[iter->assignments[iter->assignment_index]]);

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

//static bool fastcheck_hard_constraints_partial(const solution *sol,
//                                               int c1, int r2, int d2, int s2) {
//
//}

static bool fastcheck_hard_constraints(const solution *sol,
                                       int c1, int r1, int d1, int s1,
                                       int c2, int r2, int d2, int s2) {


//    return
//        fastcheck_hard_constraints_partial(sol, c1, r2, d2, s2) &&
//        fastcheck_hard_constraints_partial(sol, c2, r1, d2, s1);
    const model *model = sol->model;
    CRDSQT

    const bool same_period = d1 == d2 && s1 == s2;
    const bool same_room = r1 == r2;
    const bool same_course = c2 >= 0 && c1 == c2;
    const bool same_teacher = c2 >= 0 && model_same_teacher(model, c1, c2);
//    const bool same_curricula = c2 >= 0 && model_share_curricula(model, c1, c2);

    debug2("c1 = %d -> %s | r1 = %d -> %s | d1 = %d | s1 = %d | c2 = %d -> %s | r2 = %d -> %s | d2 = %d | s2 = %d ",
           c1, model->courses[c1].id, r1, model->rooms[r1].id, d1, s1,
           c2, c2 >= 0 ? model->courses[c2].id : "(none)", r2, model->rooms[r2].id, d2, s2);
    debug2("same_period = %d", same_period);
    debug2("same_course = %d", same_course);
    debug2("same_room = %d", same_room);
    debug2("same_teacher = %d", same_teacher);
//    debug2("same_curricula = %d", same_curricula);

    if (same_period && same_course && same_room)
        return true; // nothing to do

    // H1b
    debug2("Check H1b (c=%d, d=%d, s=%d)", c1, d2, s2);
    if (sol->helper.sum_cds[INDEX3(c1, C, d2, D, s2, S)] - same_period - same_course > 0)
        return false;

    if (c2 >= 0) {
        debug2("Check H1b (c=%d, d=%d, s=%d)", c2, d1, s1);
        if (sol->helper.sum_cds[INDEX3(c2, C, d1, D, s1, S)] - same_period - same_course > 0)
            return false;
    }

    // H2
/*  NOT NEEDED?
    debug2("Check H2 (r=%d, d=%d, s=%d)", r2, d2, s2);
    if (sol->helper.sum_rds[INDEX3(r2, R, d2, D, s2, S)] - same_period - same_room > 0)
        return false;

    if (c2 >= 0) {
        debug2("Check H2 (r=%d, d=%d, s=%d)", r1, d1, s1);
        if (sol->helper.sum_rds[INDEX3(r1, R, d1, D, s1, S)] - same_period - same_room > 0)
            return false;
    }
*/
    // H3a
/*
    int c1_n_curriculas;
    int * c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
    for (int c1q = 0; c1q < c1_n_curriculas; c1q++) {
        const int q = c1_curriculas[c1q];
        debug2("Check H3a (q=%d, d=%d, s=%d)", q, d2, s2);
        debug2("sol->helper.sum_qds[INDEX3(q, Q, d2, D, s2, S)] = %d", sol->helper.sum_qds[INDEX3(q, Q, d2, D, s2, S)]);
        for (int r = 0; r < R; r++) {
            int c_rds = sol->helper.c_rds[INDEX3(r, R, d2, D, s2, S)];
            if (c_rds) {
                int cx_n_curriculas;
                int * cx_curriculas = model_curriculas_of_course(model, c_rds, &cx_n_curriculas);
                for (int cxq = 0; cxq < cx_n_curriculas; cxq++) {
                    const int qx = c1_curriculas[cxq];

                    if (q1 == qx)
                        return false;
                }
            }

        }
    }
*/
    int c1_n_curriculas;
    int * c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
    for (int cq = 0; cq < c1_n_curriculas; cq++) {
        const int q = c1_curriculas[cq];
        bool share_curricula = c2 >= 0 && model_share_curricula(model, c1, c2, q);
        debug2("Check H3a (q=%d, d=%d, s=%d) -> %d", q, d2, s2, sol->helper.sum_qds[INDEX3(q, Q, d2, D, s2, S)]);
//        debug2("  %s %s share %s => %d", model->courses[c1].id, model->courses[c2].id, model->curriculas[q].id, model_share_curricula(model, c1, c2, q));
        if (sol->helper.sum_qds[INDEX3(q, Q, d2, D, s2, S)] - same_period - share_curricula > 0)
            return false;
    }

    if (c2 >= 0) {
        int c2_n_curriculas;
        int * c2_curriculas = model_curriculas_of_course(model, c2, &c2_n_curriculas);
        for (int cq = 0; cq < c2_n_curriculas; cq++) {
            const int q = c2_curriculas[cq];
            bool share_curricula = model_share_curricula(model, c1, c2, q);

            debug2("Check H3a (q=%d, d=%d, s=%d)", q, d1, s1);
//            debug2("  %s %s share %s => %d", model->courses[c1].id, model->courses[c2].id, model->curriculas[q].id, model_share_curricula(model, c1, c2, q));
            if (sol->helper.sum_qds[INDEX3(q, Q, d1, D, s1, S)] - same_period - share_curricula > 0)
                return false;
        }
    }

    // H3b
    const int t1 = model_teacher_by_id(model, model->courses[c1].teacher_id)->index;
    debug2("Check H3b (t=%d, d=%d, s=%d)", t1, d2, s2);
    if (sol->helper.sum_tds[INDEX3(t1, T, d2, D, s2, S)] - same_period - same_teacher > 0)
        return false;

    if (c2 >= 0) {
        const int t2 = model_teacher_by_id(model, model->courses[c2].teacher_id)->index;
        debug2("Check H3b (t=%d, d=%d, s=%d)", t2, d1, s1);
        if (sol->helper.sum_tds[INDEX3(t2, T, d1, D, s1, S)] - same_period - same_teacher > 0)
            return false;
    }

    // H4
    debug2("Check H4 (c=%d, d=%d, s=%d)", c1, d2, s2);
    if (!model_course_is_available_on_period(model, c1, d2, s2))
        return false;

    if (c2 >= 0) {
        debug2("Check H4 (c=%d, d=%d, s=%d)", c2, d1, s1);
        if (!model_course_is_available_on_period(model, c2, d1, s1))
            return false;
    }


    return true;
}

bool neighbourhood_swap_assignment(const solution *sol_in, solution *sol_out, int c1,
                                   int r1, int d1, int s1,
                                   int r2, int d2, int s2) {
    const model *model = sol_in->model;
    CRDSQT

#if DEBUG
    if (!solution_get(sol_in, c1, r1, d1, s1)) {
        eprint("Expected true in timetable");
        exit(0);
    }
#endif

    // Check if there is already a course in the picked (room,day,slot)
    int c_star = sol_in->helper.c_rds[INDEX3(r2, R, d2, D, s2, S)];


    if (c_star >= 0) {
        debug2("Found course %s scheduled in slot (%s, %d, %d)",
                sol_in->model->courses[c_star].id,
                sol_in->model->rooms[r2].id, d2, s2);
    } else {
        debug2("No course found in slot (%s, %d, %d)",
              sol_in->model->rooms[r2].id, d2, s2);
    }

    bool satisfy_hard = fastcheck_hard_constraints(sol_in, c1, r1, d1, s1, c_star, r2, d2, s2);

    // Satisfy hard constraints, do swaps
    solution_copy(sol_out, sol_in);

    // Swap assignments
    solution_set(sol_out, c1, r1, d1, s1, false);

    if (c_star >= 0)
        solution_set(sol_out, c_star, r2, d2, s2, false);

    solution_set(sol_out, c1, r2, d2, s2, true);

    if (c_star >= 0)
        solution_set(sol_out, c_star, r1, d1, s1, true);

    return satisfy_hard;
}
/*
void neighbourhood_swap_assignment(const solution *sol_in, solution *sol_out, int c1,
                                   int r1, int d1, int s1,
                                   int r2, int d2, int s2) {

#if DEBUG
    if (!solution_get(sol_in, c1, r1, d1, s1)) {
        eprint("Expected true in timetable");
        exit(0);
    }
#endif

    int c_star = -1;
    // Check if there is already a course in the picked (room,day,slot)
    for (int c = 0; c < sol_in->model->n_courses; c++) {
        if (solution_get(sol_in, c, r2, d1, s1)) {
            c_star = c;
            break;
        }
    }

    solution_copy(sol_out, sol_in);

    // Swap assignments
    solution_set(sol_out, c1, r1, d1, s1, false);
    solution_set(sol_out, c1, r2, d2, s2, true);

    if (c_star >= 0) {
        debug2("Found course %s scheduled in slot (%s, %d, %d)",
                sol_in->model->courses[c_star].id,
                sol_in->model->rooms[r2].id, d2, s2);
        solution_set(sol_out, c_star, r2, d2, s2, false);
        solution_set(sol_out, c_star, r1, d1, s1, true);
    } else {
        debug2("No course found in slot (%s, %d, %d)",
              sol_in->model->rooms[r2].id, d2, s2);
    }
}
*/