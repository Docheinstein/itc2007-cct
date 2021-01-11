#include <log/debug.h>
#include <utils/io_utils.h>
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

void neighbourhood_iter_init(neighbourhood_iter *iter, const solution *sol) {
    const int C = sol->model->n_courses;
    const int R = sol->model->n_rooms;
    const int D = sol->model->n_days;
    const int S = sol->model->n_slots;
    size_t ttsize = C * R * D * S;

    iter->solution = sol;

    iter->assignments = mallocx(ttsize, sizeof(int));
    // Find the assignments in the timetable
    int cursor = 0;
    for (int i = 0; i < ttsize; i++) {
        if (solution_get_at(sol, i))
            iter->assignments[cursor++] = i;
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

    const int C = iter->solution->model->n_courses;
    const int R = iter->solution->model->n_rooms;
    const int D = iter->solution->model->n_days;
    const int S = iter->solution->model->n_slots;

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

void neighbourhood_swap_assignment(const solution *sol_in, solution *sol_out, int c1,
                                   int r1, int d1, int s1,
                                   int r2, int d2, int s2) {

#if DEBUG
    if (!solution_get(sol_in, c1, r1, d1, s1)) {
        eprint("Expected true in timetable");
        exit(0);
    }
#endif

    if (solution_get(sol_in, c1, r2, d2, s2))
        return; // avoid to swap two lectures of the same course


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
