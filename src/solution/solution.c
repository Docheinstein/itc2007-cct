#include <utils/str_utils.h>
#include <utils/io_utils.h>
#include <utils/assert_utils.h>
#include <renderer/renderer.h>
#include "solution.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "log/debug.h"
#include "log/verbose.h"

const int ROOM_CAPACITY_COST_FACTOR = 1;
const int MIN_WORKING_DAYS_COST_FACTOR = 5;
const int CURRICULUM_COMPACTNESS_COST_FACTOR = 2;
const int ROOM_STABILITY_COST_FACTOR = 1;

void solution_init(solution *sol, const model *m) {
    MODEL(m);
    static int solution_id = 0;
    sol->_id = solution_id++;

    debug2("Initializing solution {%d}", sol->_id);

    sol->model = model;

    sol->timetable_crds = mallocx(C * R * D * S, sizeof(bool));
    sol->timetable_cdsr = mallocx(C * D * S * R, sizeof(bool));
    sol->timetable_rdsc = mallocx(R * D * S * C, sizeof(bool));
    sol->timetable_qdscr = mallocx(Q * D * S * C * R, sizeof(bool));
    sol->timetable_tdscr = mallocx(T * D * S * C * R, sizeof(bool));

    sol->c_rds = mallocx(R * D * S, sizeof(int));
    sol->r_cds = mallocx(C * D * S, sizeof(int));
    sol->l_rds = mallocx(R * D * S, sizeof(int));

    sol->sum_cr = mallocx(C * R, sizeof(int));
    sol->sum_cds = mallocx(C * D * S, sizeof(int));
    sol->sum_cd = mallocx(C * D, sizeof(int));
    sol->sum_rds = mallocx(R * D * S, sizeof(int));
    sol->sum_qds = mallocx(Q * D * S, sizeof(int));
    sol->sum_tds = mallocx(T * D * S, sizeof(int));

    sol->assignments = mallocx(model->n_lectures, sizeof(assignment));

    solution_clear(sol);
}

void solution_clear(solution *sol) {
    MODEL(sol->model);
    debug2("Clearing solution {%d}", sol->_id);

    memset(sol->timetable_crds, 0, C * R * D * S * sizeof(bool));
    memset(sol->timetable_cdsr, 0, C * D * S * R * sizeof(bool));
    memset(sol->timetable_rdsc, 0, R * D * S * C * sizeof(bool));
    memset(sol->timetable_qdscr, 0, Q * D * S * C * R * sizeof(bool));
    memset(sol->timetable_tdscr, 0, T * D * S * C * R * sizeof(bool));

    memset(sol->c_rds, -1, R * D * S * sizeof(int));
    memset(sol->r_cds, -1, C * D * S * sizeof(int));
    memset(sol->l_rds, -1, R * D * S * sizeof(int));

    memset(sol->sum_cr, 0, C * R * sizeof(int));
    memset(sol->sum_cds, 0, C * D * S * sizeof(int));
    memset(sol->sum_cd, 0, C * D * sizeof(int));
    memset(sol->sum_rds, 0, R * D * S * sizeof(int));
    memset(sol->sum_qds, 0, Q * D * S * sizeof(int));
    memset(sol->sum_tds, 0, T * D * S * sizeof(int));

    memset(sol->assignments, -1, model->n_lectures * sizeof(assignment));
}

void solution_destroy(solution *sol) {
    debug2("Destroying solution {%d}", sol->_id);

    free(sol->timetable_crds);
    free(sol->timetable_cdsr);
    free(sol->timetable_rdsc);
    free(sol->timetable_qdscr);
    free(sol->timetable_tdscr);

    free(sol->c_rds);
    free(sol->r_cds);
    free(sol->l_rds);

    free(sol->sum_cr);
    free(sol->sum_cds);
    free(sol->sum_cd);
    free(sol->sum_rds);
    free(sol->sum_qds);
    free(sol->sum_tds);

    free(sol->assignments);
}

void solution_copy(solution *sol_dest, const solution *sol_src) {
    MODEL(sol_src->model);
    debug2("Copying solution {%d} -> %d", sol_src->_id, sol_dest->_id);

    sol_dest->model = model; // should already be the same

    memcpy(sol_dest->timetable_crds, sol_src->timetable_crds, 
           C * R * D * S * sizeof(bool));
    memcpy(sol_dest->timetable_cdsr, sol_src->timetable_cdsr,
           C * D * S * R * sizeof(bool));
    memcpy(sol_dest->timetable_rdsc, sol_src->timetable_rdsc,
           R * D * S * C * sizeof(bool));
    memcpy(sol_dest->timetable_qdscr, sol_src->timetable_qdscr,
           Q * D * S * C * R * sizeof(bool));
    memcpy(sol_dest->timetable_tdscr, sol_src->timetable_tdscr,
           T * D * S * C * R * sizeof(bool));

    memcpy(sol_dest->c_rds, sol_src->c_rds,
           R * D * S * sizeof(int));
    memcpy(sol_dest->r_cds, sol_src->r_cds,
           C * D * S * sizeof(int));
    memcpy(sol_dest->l_rds, sol_src->l_rds,
           R * D * S * sizeof(int));

    memcpy(sol_dest->sum_cds, sol_src->sum_cds,
           C * D * S * sizeof(int));
    memcpy(sol_dest->sum_rds, sol_src->sum_rds,
           R * D * S * sizeof(int));
    memcpy(sol_dest->sum_qds, sol_src->sum_qds,
           Q * D * S * sizeof(int));
    memcpy(sol_dest->sum_tds, sol_src->sum_tds,
           T * D * S * sizeof(int));
    memcpy(sol_dest->sum_cr, sol_src->sum_cr,
           C * R * sizeof(int));
    memcpy(sol_dest->sum_cd, sol_src->sum_cd,
           C * D * sizeof(int));

    memcpy(sol_dest->assignments, sol_src->assignments,
           model->n_lectures * sizeof(assignment));
}

static void solution_update(solution *sol, int l, int c, int r, int d, int s, bool yes) {
    if (l < 0 || c < 0 || r < 0 || d < 0 || s < 0)
        return;

    MODEL(sol->model);

    int n_curriculas;
    int *curriculas = model_curriculas_of_course(sol->model, c, &n_curriculas);
    int t = sol->model->courses[c].teacher->index;

    sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)] = yes;

    sol->c_rds[INDEX3(r, R, d, D, s, S)] = yes ? c : -1;
    sol->r_cds[INDEX3(c, C, d, D, s, S)] = yes ? r : -1;
    sol->l_rds[INDEX3(r, R, d, D, s, S)] = yes ? l : -1;


    sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)] = yes;
    sol->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)] = yes;
    sol->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)] = yes;

    sol->sum_cr[INDEX2(c, C, r, R)] += yes ? 1 : -1;
    sol->sum_cd[INDEX2(c, C, d, D)] += yes ? 1 : -1;
    sol->sum_cds[INDEX3(c, C, d, D, s, S)] += yes ? 1 : -1;
    sol->sum_rds[INDEX3(r, R, d, D, s, S)] += yes ? 1 : -1;
    sol->sum_tds[INDEX3(t, T, d, D, s, S)] += yes ? 1 : -1;

    for (int i = 0; i < n_curriculas; i++) {
        int q = curriculas[i];
        sol->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)] = yes;
        sol->sum_qds[INDEX3(q, Q, d, D, s, S)] += yes ? 1 : -1;
        debug2("sum_qds[%d][%d][%d]=%d", q, d, s, sol->sum_qds[INDEX3(q, Q, d, D, s, S)]);
    }

    debug2("tt_crds[%d][%d][%d][%d]=%d", c, r, d, s, sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]);
    debug2("tt_cdsr[%d][%d][%d][%d]=%d", c, d, s, r, sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)]);
    debug2("c_rds[%d][%d][%d]=%d", r, d, s, sol->c_rds[INDEX3(r, R, d, D, s, S)]);
    debug2("r_cds[%d][%d][%d]=%d", c, d, s, sol->r_cds[INDEX3(c, c, d, D, s, S)]);
    debug2("sum_rds[%d][%d][%d]=%d", r, d, s, sol->sum_rds[INDEX3(r, R, d, D, s, S)]);
}

//
//void solution_set_lecture_assignment(solution *sol, int l1, int r2, int d2, int s2) {
//    MODEL(sol->model);
//    assert(l1 >= 0 && l1 < L && r2 >= 0 && r2 < R && d2 >= 0 && d2 < D && s2 >= 0 && s2 < S);
//
//    int l2 = sol->l_rds[INDEX3(r2, R, d2, D, s2, S)];
//    int c2 = l2 >= 0 ? model->lectures[l2].course->index : -1;
//
//    int c1 = model->lectures[l1].course->index;
//    int r1, d1, s1;
//    solution_get_lecture_assignment(sol, l1, &r1, &d1, &s1);
//
//    debug2("Updating solution {%d}: [lecture %d (%d:%s)] assigned from "
//           "(r=%d:%s, d=%d, s=%d) to (r=%d:%s, d=%d, s=%d) [lecture %d (%d:%s)]",
//           sol->_id,
//           l1, c1, model->courses[c1].id,
//           r1, r1 >= 0 ? model->rooms[r1].id : "-",
//           d1, s1,
//           r2, model->rooms[r2].id, d2, s2,
//           l2, c2, c2 >= 0 ? model->courses[c2].id : "-");
//
//    solution_update(sol, l1, c1, r1, d1, s1, false);
//    solution_update(sol, l2, c2, r2, d2, s2, false);
//    solution_update(sol, l1, c1, r2, d2, s2, true);
//    solution_update(sol, l2, c2, r1, d1, s1, true);
//
//    assignment *a1 = &sol->assignments[l1];
//    a1->r = r2;
//    a1->d = d2;
//    a1->s = s2;
//
//    if (l2 >= 0) {
//        assignment *a2 = &sol->assignments[l2];
//        a2->r = r1;
//        a2->d = d1;
//        a2->s = s1;
//    }
//
//    solution_assert_consistency(sol);
//}

void solution_assign_lecture(solution *sol, int l1, int r2, int d2, int s2) {
    assert(l1 >= 0 && l1 < sol->model->n_lectures && r2 >= 0 && r2 < sol->model->n_rooms &&
           d2 >= 0 && d2 < sol->model->n_days && s2 >= 0 && s2 < sol->model->n_slots);

    int c1 = sol->model->lectures[l1].course->index;
    assignment *a1 = &sol->assignments[l1];

    debug2("Updating solution {%d}: assigning [lecture %d (%d:%s)] to (r=%d:%s, d=%d, s=%d)",
           sol->_id,
           l1, c1, sol->model->courses[c1].id,
           r2, r2 >= 0 ? sol->model->rooms[r2].id : "-",
           d2, s2);

    solution_update(sol, l1, c1, r2, d2, s2, true);
    a1->r = r2;
    a1->d = d2;
    a1->s = s2;

    solution_assert_consistency(sol);
}

void solution_unassign_lecture(solution *sol, int l) {
    assert(l >= 0 && l < sol->model->n_lectures);

    int c = sol->model->lectures[l].course->index;
    assignment *a = &sol->assignments[l];

    debug2("Updating solution {%d}: unassigning [lecture %d (%d:%s)] previously in (r=%d:%s, d=%d, s=%d)",
           sol->_id,
           l, c, sol->model->courses[c].id,
           a->r, a->r >= 0 ? sol->model->rooms[a->r].id : "-",
           a->d, a->s);

    solution_update(sol, l, c, a->r, a->d, a->s, false);
    a->r = -1;
    a->d = -1;
    a->s = -1;

    solution_assert_consistency(sol);
}

void solution_get_lecture_assignment(const solution *sol, int l, int *r, int *d, int *s) {
    const assignment *a = &sol->assignments[l];
    *r = a->r;
    *d = a->d;
    *s = a->s;
}

char * solution_to_string(const solution *sol) {
    MODEL(sol->model);
    
    char *buffer = NULL;
    size_t buflen;


    FOR_L {
        const assignment *a = &sol->assignments[l];
        strappend_realloc(&buffer, &buflen,
                          "%s %s %d %d\n",
                          sol->model->lectures[l].course->id,
                          sol->model->rooms[a->r].id,
                          a->d, a->s);
    }

    if (buffer)
        buffer[strlen(buffer) - 1] = '\0';

    return buffer;
}

bool solution_satisfy_hard_constraints(const solution *sol) {
    bool h1 = solution_satisfy_lectures(sol);
    bool h2 = solution_satisfy_room_occupancy(sol);
    bool h3 = solution_satisfy_conflicts(sol);
    bool h4 = solution_satisfy_availabilities(sol);
    return h1 && h2 && h3 && h4;
}

bool solution_satisfy_lectures(const solution *sol) {
    return solution_lectures_violations(sol) == 0;
}

bool solution_satisfy_room_occupancy(const solution *sol) {
    return solution_room_occupancy_violations(sol) == 0;
}


bool solution_satisfy_availabilities(const solution *sol) {
    return solution_availabilities_violations(sol) == 0;
}

bool solution_satisfy_conflicts(const solution *sol) {
    return solution_conflicts_violations(sol) == 0;
}

static int solution_hard_constraint_lectures_violations_a(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);
    int violations = 0;

    FOR_C {
        int n = 0;
        FOR_R {
            FOR_D {
                FOR_S {
                    n += sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }
        }
        const course *course = &sol->model->courses[c];
        int delta = course->n_lectures - n;
        if (delta > 0) {
            debug2("H1 [Lectures] violation: %d lectures assigned instead of %d "
                  "for course '%s'",
                  n, course->n_lectures, course->id);
            if (strout && *strout)
                strappend_realloc(strout, strsize,
                  "H1 [Lectures] violation: %d lectures assigned instead of %d "
                  "for course '%s'\n",
                  n, course->n_lectures, course->id);
            violations += delta;
        }
    }

    return violations;
}

static int solution_hard_constraint_lectures_violations_b(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int violations = 0;
    int *room_usage = callocx(sol->model->n_courses * sol->model->n_days * sol->model->n_slots, sizeof(int));

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    room_usage[INDEX3(c, C, d, D, s, S)] += sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }
        }
    }

    FOR_C {
         FOR_D {
             FOR_S {
                 int n = room_usage[INDEX3(c, C, d, D, s, S)];
                 if (!(n <= 1)) {
                    debug2("H1 [Lectures] violation: %d rooms used for course '%s' "
                          "at (day=%d, slot=%d)",
                          n, sol->model->courses[c].id, d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                          "H1 [Lectures] violation: %d rooms used for course '%s' "
                          "at (day=%d, slot=%d)\n",
                          n, sol->model->courses[c].id, d, s);
                    violations++;
                }
             }
         }
    };
    free(room_usage);

    return violations;
}

static int solution_lectures_violations_dump(const solution *sol, char **strout, size_t *strsize) {
    return
        solution_hard_constraint_lectures_violations_a(sol, strout, strsize) +
        solution_hard_constraint_lectures_violations_b(sol, strout, strsize);
}

int solution_lectures_violations(const solution *sol) {
    return solution_lectures_violations_dump(sol, NULL, NULL);
}

static int solution_room_occupancy_violations_dump(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int violations = 0;

    FOR_R {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    n += sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
                if (!(n <= 1)) {
                    debug2("H2 [RoomOccupancy] violation: %d courses in room '%s' "
                          "at (day=%d, slot=%d)",
                          n, sol->model->rooms[r].id, d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                        "H2 [RoomOccupancy] violation: %d courses in room '%s' "
                        "at (day=%d, slot=%d)\n",
                        n, sol->model->rooms[r].id, d, s);
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_room_occupancy_violations(const solution *sol) {
    return solution_room_occupancy_violations_dump(sol, NULL, NULL);
}

static int solution_hard_constraint_conflicts_violations_a(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int violations = 0;

    FOR_Q {
        int q_n_courses;
        int *q_courses = model_courses_of_curricula(sol->model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                int n = 0;

                for (int qc = 0; qc < q_n_courses; qc++) {
                    for (int r = 0; r < sol->model->n_rooms; r++) {
                        n += sol->timetable_crds[INDEX4(q_courses[qc], C, r, R, d, D, s, S)];
                    }
                }

                if (!(n <= 1)) {
                    debug2("H3 [Conflicts] violation: %d courses of curriculum '%s' "
                          "scheduled at (day=%d, slot=%d)",
                          n, sol->model->curriculas[q].id, d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                        "H3 [Conflicts] violation: %d courses of curriculum '%s' "
                        "scheduled at (day=%d, slot=%d)\n",
                        n, sol->model->curriculas[q].id, d, s);
                    violations++;
                }
            }
        }
    }

    return violations;
}

static int solution_hard_constraint_conflicts_violations_b(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int violations = 0;

    FOR_T {
        int t_n_courses;
        int *t_courses = model_courses_of_teacher(sol->model, t, &t_n_courses);

        FOR_D {
            FOR_S {
                int n = 0;
                for (int tc = 0; tc < t_n_courses; tc++) {
                    FOR_R {
                        n += sol->timetable_crds[INDEX4(t_courses[tc], C, r, R, d, D, s, S)];
                    }
                }

                if (!(n <= 1)) {
                    debug2("H3 [Conflicts] violation: %d courses taught by "
                          "teacher '%s' scheduled at (day=%d, slot=%d)",
                          n, sol->model->teachers[t].id, d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                        "H3 [Conflicts] violation: %d courses taught by "
                        "teacher '%s' scheduled at (day=%d, slot=%d)\n",
                        n, sol->model->teachers[t].id, d, s);
                    violations++;
                }
            }
        }
    }

    return violations;
}

static int solution_conflicts_violations_dump(const solution *sol, char **strout, size_t *strsize) {
    return
        solution_hard_constraint_conflicts_violations_a(sol, strout, strsize) +
        solution_hard_constraint_conflicts_violations_b(sol, strout, strsize);
}

int solution_conflicts_violations(const solution *sol) {
    return solution_conflicts_violations_dump(sol, NULL, NULL);
}

static int solution_availabilities_violations_dump(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int violations = 0;

    FOR_C {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
                if (!(n <= model_course_is_available_on_period(sol->model, c, d, s))) {
                    debug2("H4 [Availabilities] violation: course '%s' scheduled %d time(s) on "
                          "(day=%d, slot=%d) but breaks unavailability constraint",
                          sol->model->courses[c].id, n, d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                            "H4 [Availabilities] violation: course '%s' scheduled %d time(s) on "
                            "(day=%d, slot=%d) but breaks unavailability constraint\n",
                            sol->model->courses[c].id, n, d, s);
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_availabilities_violations(const solution *sol) {
    return solution_availabilities_violations_dump(sol, NULL, NULL);
}

int solution_cost(const solution *sol) {
    return
            solution_room_capacity_cost(sol) +
            solution_min_working_days_cost(sol) +
            solution_curriculum_compactness_cost(sol) +
            solution_room_stability_cost(sol);
}

static int solution_room_capacity_cost_dump(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int penalties = 0;

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]) {
                        int delta = sol->model->courses[c].n_students - sol->model->rooms[r].capacity;
                        if (delta > 0) {
                            debug2("S1(%d) [RoomCapacity] penalty: course '%s' has %d"
                                  " students but it's scheduled in room '%s' with %d "
                                  "seats at (day=%d, slot=%d)",
                                   delta * ROOM_CAPACITY_COST_FACTOR,
                                  sol->model->courses[c].id, sol->model->courses[c].n_students,
                                  sol->model->rooms[r].id, sol->model->rooms[r].capacity,
                                  d, s);
                            if (strout && *strout)
                                strappend_realloc(strout, strsize,
                                    "S1(%d) [RoomCapacity] penalty: course '%s' has %d"
                                    " students but it's scheduled in room '%s' with %d "
                                    "seats at (day=%d, slot=%d)\n",
                                                  delta * ROOM_CAPACITY_COST_FACTOR,
                                    sol->model->courses[c].id, sol->model->courses[c].n_students,
                                    sol->model->rooms[r].id, sol->model->rooms[r].capacity,
                                    d, s);
                            penalties += delta;
                        }
                    }
                }
            }
        }
    };

    return penalties * ROOM_CAPACITY_COST_FACTOR;
}

int solution_room_capacity_cost(const solution *sol) {
    return solution_room_capacity_cost_dump(sol, NULL, NULL);
}

static int solution_min_working_days_cost_dump(const solution *sol, char **strout, size_t *strsize) {
    MODEL(sol->model);

    int penalties = 0;

    FOR_C {
        int sum_y_cd = 0;

        FOR_D {
            int y_cd = 0;

            FOR_R {
                FOR_S {
                    y_cd = y_cd || sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }

            sum_y_cd += y_cd;
        }

        int delta = sol->model->courses[c].min_working_days - sum_y_cd;
        if (delta > 0) {
            debug2("S2(%d) [MinWorkingDays] penalty: course '%s' spread among %d different days "
                  "but Minimum Working Days are %d",
                   delta * MIN_WORKING_DAYS_COST_FACTOR,
                  sol->model->courses[c].id, sum_y_cd,
                  sol->model->courses[c].min_working_days);
            if (strout && *strout)
                strappend_realloc(strout, strsize,
                    "S2(%d) [MinWorkingDays] penalty: course '%s' spread among %d different days "
                    "but Minimum Working Days are %d\n",
                                  delta * MIN_WORKING_DAYS_COST_FACTOR,
                    sol->model->courses[c].id, sum_y_cd,
                    sol->model->courses[c].min_working_days);

            penalties += delta;
        }
    };

    return penalties * MIN_WORKING_DAYS_COST_FACTOR;
}

int solution_min_working_days_cost(const solution *sol) {
    return solution_min_working_days_cost_dump(sol, NULL, NULL);
}

static int solution_curriculum_compactness_cost_dump(const solution *sol,
                                                     char **strout, size_t *strsize) {
    MODEL(sol->model);

    int penalties = 0;

    int *slots = mallocx(sol->model->n_slots, sizeof(int));

    FOR_Q {
        int q_n_courses;
        int *q_courses = model_courses_of_curricula(sol->model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                int z_qds = 0;

                for (int qc = 0; qc < q_n_courses; qc++) {
                    for (int r = 0; r < sol->model->n_rooms; r++) {
                        z_qds += sol->timetable_crds[INDEX4(q_courses[qc], C, r, R, d, D, s, S)];
                    }
                }

                slots[s] = z_qds;
            }

            FOR_S {
                bool prev = s > 0 && slots[s - 1];
                bool cur = slots[s];
                bool next = s < sol->model->n_slots - 1 && slots[s + 1];

                if (cur && !prev && !next) {
                    debug2("S3(%d) [CurriculumCompactness] penalty: curriculum '%s' "
                          "has %d isolated lecture(s) at (day=%d, slot=%d)",
                           CURRICULUM_COMPACTNESS_COST_FACTOR,
                           sol->model->curriculas[q].id, slots[s], d, s);
                    if (strout && *strout)
                        strappend_realloc(strout, strsize,
                            "S3(%d) [CurriculumCompactness] penalty: curriculum '%s' "
                            "has %d isolated lecture(s) at (day=%d, slot=%d)\n",
                                          CURRICULUM_COMPACTNESS_COST_FACTOR,
                                          sol->model->curriculas[q].id, slots[s], d, s);

                    penalties += slots[s];
                }
            }
        }
    };

    free(slots);
    return penalties * CURRICULUM_COMPACTNESS_COST_FACTOR;
}

int solution_curriculum_compactness_cost(const solution *sol) {
    return solution_curriculum_compactness_cost_dump(sol, NULL, NULL);
}

static int solution_room_stability_cost_dump(const solution *sol,
                                             char **strout, size_t *strsize) {
    MODEL(sol->model);

    int penalties = 0;

    FOR_C {
        int sum_w_cr = 0;

        FOR_R {
            int w_cr = 0;

            FOR_D {
                FOR_S {
                    w_cr = w_cr || sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
            };

            sum_w_cr += w_cr;
        }

        int delta = sum_w_cr - 1;
        if (delta > 0) {
            debug2("S4(%d) [RoomStability] penalty: course '%s' uses %d rooms",
                   delta * ROOM_STABILITY_COST_FACTOR,
                  sol->model->courses[c].id, sum_w_cr);
            if (strout && *strout)
                strappend_realloc(strout, strsize,
                    "S4(%d) [RoomStability] penalty: course '%s' uses %d rooms\n",
                                  delta * ROOM_STABILITY_COST_FACTOR,
                    sol->model->courses[c].id, sum_w_cr);

            penalties += delta;
        }
    };

    return penalties * ROOM_STABILITY_COST_FACTOR;
}

int solution_room_stability_cost(const solution *sol) {
    return solution_room_stability_cost_dump(sol, NULL, NULL);
}

char *solution_quality_to_string(const solution *sol, bool verbose) {
    char *str = NULL;
    size_t strsize;

    if (verbose)
        strappend_realloc(&str, &strsize, "");

    int h1 = solution_lectures_violations_dump(sol, &str, &strsize);
    int h2 = solution_room_occupancy_violations_dump(sol, &str, &strsize);
    int h3 = solution_conflicts_violations_dump(sol, &str, &strsize);
    int h4 = solution_availabilities_violations_dump(sol, &str, &strsize);
    int s1 = solution_room_capacity_cost_dump(sol, &str, &strsize);
    int s2 = solution_min_working_days_cost_dump(sol, &str, &strsize);
    int s3 = solution_curriculum_compactness_cost_dump(sol, &str, &strsize);
    int s4 = solution_room_stability_cost_dump(sol, &str, &strsize);

    strappend_realloc(&str, &strsize, str ? "\n" : "");

    strappend_realloc(&str, &strsize,
        "Violations (%d)\n"
        "  Lectures:                    %d\n"
        "  RoomOccupancy:               %d\n"
        "  Conflicts violations:        %d\n"
        "  Availabilities violations:   %d\n"
        "Cost (%d)\n"
        "  RoomCapacity cost:           %d\n"
        "  MinWorkingDays cost:         %d\n"
        "  CurriculumCompactness cost:  %d\n"
        "  RoomStability cost:          %d",
        h1 + h2 + h3 + h4,
        h1,
        h2,
        h3,
        h4,
        s1 + s2 + s3 + s4,
        s1,
        s2,
        s3,
        s4
    );

    return str;
}

bool write_solution(const solution *sol, const char *filename) {
    if (!filename)
        return false;

    char *sol_str = solution_to_string(sol);

    bool success = filewrite(filename, false, sol_str);
    if (!success)
        eprint("ERROR: failed to write output solution to '%s' (%s)",
               filename, strerror(errno));

    free(sol_str);

    return success;
}

unsigned long long solution_fingerprint(const solution *sol) {
    MODEL(sol->model);
    unsigned long long h = 0;
    for (int i = 0; i < C * R * D * S; i++) {
        h += sol->timetable_crds[i];
        h *= 85637481940593;
    }
    /*
    char *ptr = (char *) sol->assignments;
    size_t len = L * sizeof(assignment);
    int i = 0;
    while (i < len) {
        h += *ptr;
        h *= 85637481940593;
        ptr++;
        i++;
    }
    */
    return h;
}

void solution_assert_consistency(const solution *sol) {
#ifdef ASSERT
    solution_assert_consistency_real(sol);
#endif
}

void solution_assert_consistency_real(const solution *sol) {
    MODEL(sol->model);

    // timetable_crds;
    // timetable_cdsr;
    // timetable_rdsc;
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    bool b1 = sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                    bool b2 = sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)];
                    bool b3 = sol->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)];
                    assert_real(b1 == b2);
                    assert_real(b2 == b3);
                }
            }
        }
    }

    // timetable_qdscr
    FOR_Q {
        FOR_C {
            FOR_R {
                FOR_D {
                    FOR_S {
                        if (sol->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)])
                            assert_real(sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]);
                    }
                }
            }
        }
    };

    // timetable_tdscr
    FOR_T {
        FOR_C {
            FOR_R {
                FOR_D {
                    FOR_S {
                        if (sol->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)])
                            assert_real(sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]);
                    }
                }
            }
        }
    };

    // c_rds (1)
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]) {
                        assert_real(sol->c_rds[INDEX3(r, R, d, D, s, S)] == c);
                    }
                }
            }
        }
    }

    // c_rds (2)
    FOR_R {
        FOR_D {
            FOR_S {
                int cc = sol->c_rds[INDEX3(r, R, d, D, s, S)];
                if (cc >= 0)
                    assert_real(sol->timetable_crds[INDEX4(cc, C, r, R, d, D, s, S)]);
                else {
                    FOR_C {
                        assert_real(!sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]);
                    }
                }
            }
        }
    }


    // r_cds (1)
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]) {
                        assert_real(sol->c_rds[INDEX3(r, R, d, D, s, S)] == c);
                    }
                }
            }
        }
    }

    // r_cds (2)
    FOR_C {
        FOR_D {
            FOR_S {
                int rr = sol->r_cds[INDEX3(c, C, d, D, s, S)];
//                print("r %d %d %d = %d", c, d, s, rr);
                if (rr >= 0)
                    assert_real(sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, rr, R)]);
                else {
                    FOR_R {
//                        print("sol->timetable_cdsr[%d %d %d %d])=%d", c, d, s, r, sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)]);
                        assert_real(!sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)]);
                    }
                }
            }
        }
    }

    // l_rds (1)
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]) {
                        int l = sol->l_rds[INDEX3(r, R, d, D, s, S)];
                        assert_real(l >= 0);
                        assignment *a = &sol->assignments[l];
                        assert_real(a->r == r);
                        assert_real(a->d == d);
                        assert_real(a->s == s);
                    }
                }
            }
        }
    }

    // l_rds (2)
    FOR_R {
        FOR_D {
            FOR_S {
                int l = sol->l_rds[INDEX3(r, R, d, D, s, S)];
                if (l >= 0) {
                    assert_real(sol->timetable_crds[INDEX4(model->lectures[l].course->index, C, r, R, d, D, s, S)]);
                }
                else {
                    FOR_C {
                        assert_real(!sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)]);
                    }
                }
            }
        }
    }

    // sum_cr
    FOR_C {
        FOR_R {
            int sum = 0;
            FOR_D {
                FOR_S {
                    sum += sol->timetable_crds[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }
            assert_real(sum == sol->sum_cr[INDEX2(c, C, r, R)]);
        }
    };

    // sum_cds
    FOR_C {
        FOR_D {
            FOR_S {
                int sum = 0;
                FOR_R {
                    sum += sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)];
                }
                assert_real(sum == sol->sum_cds[INDEX3(c, C, d, D, s, S)]);
            }
        }
    };

    // sum_cd
    FOR_C {
        FOR_D {
            int sum = 0;
            FOR_S {
                FOR_R {
                    sum += sol->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)];
                }
            }
            assert_real(sum == sol->sum_cd[INDEX2(c, C, d, D)]);
        }
    };

    // sum_rds
    FOR_R {
        FOR_D {
            FOR_S {
                int sum = 0;
                FOR_C {
                    sum += sol->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)];
                }
                assert_real(sum == sol->sum_rds[INDEX3(r, R, d, D, s, S)]);
            }
        }
    };

    // sum_qds
    FOR_Q {
        FOR_D {
            FOR_S {
                int sum = 0;
                FOR_C {
                    FOR_R {
                        sum += sol->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)];
                    }
                }
                assert_real(sum == sol->sum_qds[INDEX3(q, Q, d, D, s, S)]);
            }
        }
    };

    // sum_tds
    FOR_T {
        FOR_D {
            FOR_S {
                int sum = 0;
                FOR_C {
                    FOR_R {
                        sum += sol->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)];
                    }
                }
                assert_real(sum == sol->sum_tds[INDEX3(t, T, d, D, s, S)]);
            }
        }
    };

    // assignments
    FOR_L {
        const lecture *ll = &model->lectures[l];
        const int c = ll->course->index;
        const assignment *a = &sol->assignments[l];
        if (a->r >= 0 && a->d >= 0 && a->s >= 0) {
            assert_real(sol->timetable_crds[INDEX4(c, C, a->r, R, a->d, D, a->s, S)]);
            assert_real(sol->l_rds[INDEX3(a->r, R, a->d, D, a->s, S)] == l);
        }
    }
}

void solution_assert(const solution *sol, bool expected_feasibility, int expected_cost) {
#ifdef ASSERT
    solution_assert_real(sol, expected_feasibility, expected_cost);
#endif
}

void solution_assert_real(const solution *sol, bool expected_feasibility, int expected_cost) {
    solution_assert_consistency_real(sol);
    assert_real(expected_feasibility == solution_satisfy_hard_constraints(sol));
    if (expected_cost >= 0)
        assert_real(expected_cost == solution_cost(sol));
}
