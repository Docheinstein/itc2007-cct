#include <utils/str_utils.h>
#include <utils/io_utils.h>
#include "solution.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "log/debug.h"
#include "log/verbose.h"
#include "utils/def_utils.h"
#include "config.h"


static void solution_helper_init(solution_helper *helper, const model *model) {
    CRDSQT(model)
    helper->valid = false;

    helper->c_rds = mallocx(R * D * S, sizeof(int));
    helper->r_cds = mallocx(C * D * S, sizeof(int));

    helper->sum_cr = mallocx(C * R, sizeof(int));

    helper->timetable_cdsr = mallocx(C * D * S * R, sizeof(bool));
    helper->sum_cds = mallocx(C * D * S, sizeof(int));
    helper->sum_cd = mallocx(C * D, sizeof(int));

    helper->timetable_rdsc = mallocx(R * D * S * C, sizeof(bool));
    helper->sum_rds = mallocx(R * D * S, sizeof(int));

    helper->timetable_qdscr = mallocx(Q * D * S * C * R, sizeof(bool));
    helper->sum_qds = mallocx(Q * D * S, sizeof(int));

    helper->timetable_tdscr = mallocx(T * D * S * C * R, sizeof(bool));
    helper->sum_tds = mallocx(T * D * S, sizeof(int));
}

static void solution_helper_destroy(solution_helper *helper) {
    helper->valid = false;

    free(helper->c_rds);
    free(helper->r_cds);

    free(helper->sum_cr);

    free(helper->timetable_cdsr);
    free(helper->sum_cds);
    free(helper->sum_cd);

    free(helper->timetable_rdsc);
    free(helper->sum_rds);

    free(helper->timetable_qdscr);
    free(helper->sum_qds);
    
    free(helper->timetable_tdscr);
    free(helper->sum_tds);
}

static void solution_helper_compute(solution_helper *helper, const solution *sol) {
    CRDSQT(sol->model)
    const model *model = sol->model;

    debug2("@@ solution_helper_compute @@");
    helper->valid = true;

    memset(helper->c_rds, -1, R * D * S * sizeof(int));
    memset(helper->r_cds, -1, C * D * S * sizeof(int));

    memset(helper->sum_cr, 0, C * R * sizeof(int));

    memset(helper->timetable_cdsr, 0, C * D * S * R * sizeof(bool));
    memset(helper->sum_cds, 0, C * D * S * sizeof(int));
    memset(helper->sum_cd, 0, C * D * sizeof(int));

    memset(helper->timetable_rdsc, 0, R * D * S * C * sizeof(bool));
    memset(helper->sum_rds, 0, R * D * S * sizeof(int));

    memset(helper->timetable_qdscr, 0, Q * D * S * C * R * sizeof(bool));
    memset(helper->sum_qds, 0, Q * D * S * sizeof(int));

    memset(helper->timetable_tdscr, 0, T * D * S * C * R * sizeof(bool));
    memset(helper->sum_tds, 0, T * D * S * sizeof(int));

    // c_rds
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable[INDEX4(c, C, r, R, d, D, s, S)])
                        helper->c_rds[INDEX3(r, R, d, D, s, S)] = c;
                    debug2("helper->c_rds[INDEX3(%d, %d, %d)] = %d",
                           r, d, s, helper->c_rds[INDEX3(r, R, d, D, s, S)]);
                }
            }
        }
    }

    // r_cds
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable[INDEX4(c, C, r, R, d, D, s, S)])
                        helper->r_cds[INDEX3(c, C, d, D, s, S)] = r;
                    debug2("helper->r_cds[INDEX3(%d, %d, %d)] = %d",
                           c, d, s, helper->r_cds[INDEX3(c, C, d, D, s, S)]);
                }
            }
        }
    }

    // sum_cr
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    helper->sum_cr[INDEX2(c, C, r, R)] +=
                            sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
                debug2("helper->sum_cr[INDEX2(%d, %d)] = %d",
                       c, r, helper->sum_cr[INDEX2(c, C, r, R)]);
            }
        }
    };

    // timetable_cdsr
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    helper->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)] =
                            sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                    debug2("helper->timetable_cdsr[INDEX4(%d, %d, %d, %d)] = %d",
                           c, d, s, r, helper->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)]);
                }
            }
        }
    }

    // sum_cds
    FOR_C {
        FOR_D {
            FOR_S {
                FOR_R {
                    helper->sum_cds[INDEX3(c, C, d, D, s, S)] += 
                            helper->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)];
                }
                debug2("helper->sum_cds[INDEX3(%d, %d, %d)] = %d",
                       c, d, s, helper->sum_cds[INDEX3(c, C, d, D, s, S)]);
            }
        }
    };

    // sum_cd
    FOR_C {
        FOR_D {
            FOR_S {
                FOR_R {
                    helper->sum_cd[INDEX2(c, C, d, D)] +=
                            helper->timetable_cdsr[INDEX4(c, C, d, D, s, S, r, R)];
                }
                debug2("helper->sum_cd[INDEX2(%d, %d, %d)] = %d",
                       c, d, helper->sum_cds[INDEX2(c, C, d, D)]);
            }
        }
    };

    // timetable_rdsc
    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    helper->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)] =
                            sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                    debug2("helper->timetable_rdsc[INDEX4(%d, %d, %d, %d)] = %d",
                           r, d, s, c, helper->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)]);
                }
            }
        }
    }    
    
    // sum_rds
    FOR_R {
        FOR_D {
            FOR_S {
                FOR_C {
                    helper->sum_rds[INDEX3(r, R, d, D, s, S)] +=
                            helper->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)];
                    debug2("summing helper->timetable_rdsc[INDEX4(%s, %d, %d, %s) = %d",
                           model->rooms[r].id, d, s, model->courses[c].id,
                           helper->timetable_rdsc[INDEX4(r, R, d, D, s, S, c, C)]);
                }
                debug2("helper->sum_rds[INDEX3(%d, %d, %d)] = %d",
                       r, d, s, helper->sum_rds[INDEX3(r, R, d, D, s, S)]);
            }
        }
    }

    // timetable_qdscr
    FOR_Q {
        int q_n_courses;
        int * q_courses = model_courses_of_curricula(model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                for (int qc = 0; qc < q_n_courses; qc++) {
                    int c = q_courses[qc];
                    FOR_R {
                        helper->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)] =
                                sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                        debug2("helper->timetable_qdscr[INDEX5(%s, %d, %d, %s, %s) = %d] = %d",
                           model->curriculas[q].id, d, s, model->courses[c].id, model->rooms[r].id,
                           INDEX5(q, Q, d, D, s, S, c, C, r, R),
                           helper->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)]);
                    }
                }
            }
        }
    }
    // timetable_qdscr DUMP DEBUG
    FOR_Q {
        int q_n_courses;
        int * q_courses = model_courses_of_curricula(model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                for (int qc = 0; qc < q_n_courses; qc++) {
                    int c = q_courses[qc];
                    FOR_R {
                        debug2("DUMP timetable_qdscr[INDEX5(%s, %d, %d, %s, %s)] = %d",
                           model->curriculas[q].id, d, s, model->courses[c].id, model->rooms[r].id,
                           helper->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)]);
                    }
                }
            }
        }
    }

    // sum_qds
    FOR_Q {
        int q_n_courses;
        int * q_courses = model_courses_of_curricula(model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                for (int qc = 0; qc < q_n_courses; qc++) {
                    int c = q_courses[qc];
                    FOR_R {
                        debug2("summing timetable_qdscr[INDEX5(%s, %d, %d, %s, %s)] = %d",
                           model->curriculas[q].id, d, s, model->courses[c].id, model->rooms[r].id,
                           helper->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)]);
                        helper->sum_qds[INDEX3(q, Q, d, D, s, S)] +=
                            helper->timetable_qdscr[INDEX5(q, Q, d, D, s, S, c, C, r, R)];
                    }
                }
                debug2("helper->sum_qds[INDEX3(%s, %d, %d)] = %d",
                           model->curriculas[q].id, d, s, helper->sum_qds[INDEX3(q, Q, d, D, s, S)]);
            }
        }
    }
    
    // timetable_tdscr
    FOR_C {
        const int t = model->courses[c].teacher->index;
        FOR_R {
            FOR_D {
                FOR_S {
                    helper->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)] =
                            sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                    debug2("helper->timetable_tdscr[INDEX5(%s, %d, %d, %s, %d)] = %d",
                            model->teachers[t].id, d, s, model->courses[c].id, r, helper->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)]);
                }
            }
        }
    }

    // sum_tds
    FOR_C {
        const int t = model->courses[c].teacher->index;
        FOR_D {
            FOR_S {
                FOR_R {
                    helper->sum_tds[INDEX3(t, T, d, D, s, S)] +=
                        helper->timetable_tdscr[INDEX5(t, T, d, D, s, S, c, C, r, R)];
                }
                debug2("helper->sum_tds[INDEX3(%s, %d, %d)] = %d",
                       model->teachers[t].id, d, s, helper->sum_tds[INDEX3(t, T, d, D, s, S)]);
            }
        }
    };
    
}

static void solution_helper_copy(solution_helper *helper_dest, solution_helper *helper_src,
                                 const solution *sol) {
    CRDSQT(sol->model)
//
//    memcpy(helper_dest->timetable_cdsr, helper_src->timetable_cdsr,
//           C * D * S * R * sizeof(bool));
//    memcpy(helper_dest->timetable_rdsc, helper_src->timetable_rdsc,
//           R * D * S * C * sizeof(bool));
//    memcpy(helper_dest->timetable_qdscr, helper_src->timetable_qdscr,
//           Q * D * S * R * C * sizeof(bool));
//    memcpy(helper_dest->timetable_tdscr, helper_src->timetable_tdscr,
//           T * D * S * R * C * sizeof(bool));
//
//
//    memcpy(helper_dest->sum_cds, helper_src->timetable_cdsr,
//           C * D * S * sizeof(bool));
//    memcpy(helper_dest->sum_rds, helper_src->timetable_rdsc,
//           R * D * S * sizeof(bool));
//    memcpy(helper_dest->sum_qds, helper_src->timetable_qdscr,
//           Q * D * S * sizeof(bool));
//    memcpy(helper_dest->sum_tds, helper_src->timetable_tdscr,
//           T * D * S * sizeof(bool));

}

void solution_init(solution *sol, const model *model) {
    CRDSQT(model)
    sol->model = model;
    sol->timetable = callocx(C * R * D * S, sizeof(bool));
    sol->helper = NULL;
}

void solution_reinit(solution *sol) {
    solution_destroy(sol);
    solution_init(sol, sol->model);
}

void solution_destroy(solution *sol) {
    free(sol->timetable);
    if (sol->helper)
        solution_helper_destroy(sol->helper);
    free(sol->helper);
}

const solution_helper *solution_get_helper(solution *solution) {
    if (!solution->helper) {
        solution->helper = mallocx(1, sizeof(solution_helper));
        solution_helper_init(solution->helper, solution->model);
    }
    if (!solution->helper->valid)
        solution_helper_compute(solution->helper, solution);
    return solution->helper;
}

bool solution_invalidate_helper(solution *sol) {
    bool was_valid = sol->helper && sol->helper->valid;
    if (was_valid)
        sol->helper->valid = false;
    return was_valid;
}

void solution_copy(solution *solution_dest, const solution *solution_src) {
    memcpy(solution_dest->timetable, solution_src->timetable,
           solution_dest->model->n_courses * solution_dest->model->n_rooms *
           solution_dest->model->n_days * solution_dest->model->n_slots * sizeof(bool));
    solution_dest->model = solution_src->model; // should already be the same

//    solution_helper_copy(&solution_dest->helper, &solution_src->helper);
}

char * solution_to_string(const solution *sol) {
    CRDSQT(sol->model)
    
    char *buffer = NULL;
    size_t buflen;

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (solution_get(sol, c, r, d, s))
                        strappend_realloc(
                                &buffer, &buflen, "%s %s %d %d\n",
                                sol->model->courses[c].id, sol->model->rooms[r].id, d, s);
                }
            }
        }
    };

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
    CRDSQT(sol->model)
    int violations = 0;

    FOR_C {
        int n = 0;
        FOR_R {
            FOR_D {
                FOR_S {
                    n += solution_get(sol, c, r, d, s);
                }
            }
        }
        const course *course = &sol->model->courses[c];
        if (n != course->n_lectures) {
            debug2("H1 (Lectures) violation: %d lectures assigned instead of %d "
                  "for course '%s'",
                  n, course->n_lectures, course->id);
            if (strout)
                strappend_realloc(strout, strsize,
                  "H1 (Lectures) violation: %d lectures assigned instead of %d "
                  "for course '%s'\n",
                  n, course->n_lectures, course->id);
            violations++;
        }
    }

    return violations;
}

static int solution_hard_constraint_lectures_violations_b(const solution *sol, char **strout, size_t *strsize) {
    CRDSQT(sol->model)

    int violations = 0;
    int *room_usage = callocx(sol->model->n_courses * sol->model->n_days * sol->model->n_slots, sizeof(int));

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    room_usage[INDEX3(c, C, d, D, s, S)] += solution_get(sol, c, r, d, s);
                }
            }
        }
    }

    FOR_C {
         FOR_D {
             FOR_S {
                 int n = room_usage[INDEX3(c, C, d, D, s, S)];
                 if (!(n <= 1)) {
                    debug2("H1 (Lectures) violation: %d rooms used for course '%s' "
                          "on (day=%d, slot=%d)",
                          n, sol->model->courses[c].id, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                          "H1 (Lectures) violation: %d rooms used for course '%s' "
                          "on (day=%d, slot=%d)\n",
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
    CRDSQT(sol->model)

    int violations = 0;

    FOR_R {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    n += solution_get(sol, c, r, d, s);
                }
                if (!(n <= 1)) {
                    debug2("H2 (RoomOccupancy) violation: %d courses in room '%s' "
                          "on (day=%d, slot=%d)",
                          n, sol->model->rooms[r].id, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                        "H2 (RoomOccupancy) violation: %d courses in room '%s' "
                        "on (day=%d, slot=%d)\n",
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
    CRDSQT(sol->model)

    int violations = 0;

    FOR_Q {
        int q_n_courses;
        int * q_courses = model_courses_of_curricula(sol->model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                int n = 0;

                for (int qc = 0; qc < q_n_courses; qc++) {
                    for (int r = 0; r < sol->model->n_rooms; r++) {
                        n += solution_get(sol, q_courses[qc], r, d, s);
                    }
                }

                if (!(n <= 1)) {
                    debug2("H3 (Conflicts) violation: %d courses of curriculum '%s' "
                          "scheduled on (day=%d, slot=%d)",
                          n, sol->model->curriculas[q].id, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                        "H3 (Conflicts) violation: %d courses of curriculum '%s' "
                        "scheduled on (day=%d, slot=%d)\n",
                        n, sol->model->curriculas[q].id, d, s);
                    violations++;
                }
            }
        }
    }

    return violations;
}

static int solution_hard_constraint_conflicts_violations_b(const solution *sol, char **strout, size_t *strsize) {
    CRDSQT(sol->model)

    int violations = 0;

    FOR_T {
        int t_n_courses;
        int * t_courses = model_courses_of_teacher(sol->model, t, &t_n_courses);

        FOR_D {
            FOR_S {
                int n = 0;
                for (int tc = 0; tc < t_n_courses; tc++) {
                    FOR_R {
                        n += solution_get(sol, t_courses[tc], r, d, s);
                    }
                }

                if (!(n <= 1)) {
                    debug2("H3 (Conflicts) violation: %d courses taught by "
                          "teacher '%s' scheduled on (day=%d, slot=%d)",
                          n, sol->model->teachers[t].id, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                        "H3 (Conflicts) violation: %d courses taught by "
                        "teacher '%s' scheduled on (day=%d, slot=%d)\n",
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
    CRDSQT(sol->model)

    int violations = 0;

    FOR_C {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += solution_get(sol, c, r, d, s);
                }
                if (!(n <= model_course_is_available_on_period(sol->model, c, d, s))) {
                    debug2("H4 (Availabilities) violation: course '%s' scheduled %d time(s) on "
                          "(day=%d, slot=%d) but breaks unavailability constraint",
                          sol->model->courses[c].id, n, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                            "H4 (Availabilities) violation: course '%s' scheduled %d time(s) on "
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
    CRDSQT(sol->model)

    int penalties = 0;

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (solution_get(sol, c, r, d, s)) {
                        int delta = sol->model->courses[c].n_students - sol->model->rooms[r].capacity;
                        if (delta > 0) {
                            debug2("S1 (RoomCapacity) penalty: course '%s' has %d"
                                  " students but it's scheduled in room '%s' with %d "
                                  "seats on (day=%d, slot=%d)",
                                  sol->model->courses[c].id, sol->model->courses[c].n_students,
                                  sol->model->rooms[r].id, sol->model->rooms[r].capacity,
                                  d, s);
                            if (strout)
                                strappend_realloc(strout, strsize,
                                    "S1 (RoomCapacity) penalty: course '%s' has %d"
                                    " students but it's scheduled in room '%s' with %d "
                                    "seats on (day=%d, slot=%d)\n",
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

    return penalties * ROOM_CAPACITY_COST;
}

int solution_room_capacity_cost(const solution *sol) {
    return solution_room_capacity_cost_dump(sol, NULL, NULL);
}

static int solution_min_working_days_cost_dump(const solution *sol, char **strout, size_t *strsize) {
    CRDSQT(sol->model)

    int penalties = 0;

    FOR_C {
        int sum_y_cd = 0;

        FOR_D {
            int y_cd = 0;

            FOR_R {
                FOR_S {
                    y_cd = y_cd || solution_get(sol, c, r, d, s);
                }
            }

            sum_y_cd += y_cd;
        }

        int delta = sol->model->courses[c].min_working_days - sum_y_cd;
        if (delta > 0) {
            debug2("S2 (MinWorkingDays) penalty: course '%s' spread among %d different days "
                  "but Minimum Working Days is %d",
                  sol->model->courses[c].id, sum_y_cd,
                  sol->model->courses[c].min_working_days);
            if (strout)
                strappend_realloc(strout, strsize,
                    "S2 (MinWorkingDays) penalty: course '%s' spread among %d different days "
                    "but Minimum Working Days is %d\n",
                    sol->model->courses[c].id, sum_y_cd,
                    sol->model->courses[c].min_working_days);

            penalties += delta;
        }
    };

    return penalties * MIN_WORKING_DAYS_COST;
}

int solution_min_working_days_cost(const solution *sol) {
    return solution_min_working_days_cost_dump(sol, NULL, NULL);
}

static int solution_curriculum_compactness_cost_dump(const solution *sol,
                                                     char **strout, size_t *strsize) {
    CRDSQT(sol->model)

    int penalties = 0;

    bool * slots = mallocx(sol->model->n_slots, sizeof(bool));

    FOR_Q {
        int q_n_courses;
        int * q_courses = model_courses_of_curricula(sol->model, q, &q_n_courses);

        FOR_D {
            FOR_S {
                int z_qds = 0;

                for (int qc = 0; qc < q_n_courses && !z_qds; qc++) {
                    for (int r = 0; r < sol->model->n_rooms && !z_qds; r++) {
                        z_qds = solution_get(sol, q_courses[qc], r, d, s);
                    }
                }

                slots[s] = z_qds;
            }

            FOR_S {
                bool prev = s > 0 && slots[s - 1];
                bool cur = slots[s];
                bool next = s < sol->model->n_slots - 1 && slots[s + 1];

                if (cur && !prev && !next) {
                    debug2("S3 (CurriculumCompactness) penalty: curriculum '%s' "
                          "has a course scheduled alone on (day=%d, slot=%d) without adjacent lessons",
                          sol->model->curriculas[q].id, d, s);
                    if (strout)
                        strappend_realloc(strout, strsize,
                            "S3 (CurriculumCompactness) penalty: curriculum '%s' "
                            "has a course scheduled alone on (day=%d, slot=%d) without adjacent lessons\n",
                            sol->model->curriculas[q].id, d, s);

                    penalties++;
                }
            }
        }
    };

    free(slots);
    return penalties * CURRICULUM_COMPACTNESS_COST;
}

int solution_curriculum_compactness_cost(const solution *sol) {
    return solution_curriculum_compactness_cost_dump(sol, NULL, NULL);
}

static int solution_room_stability_cost_dump(const solution *sol,
                                             char **strout, size_t *strsize) {
    CRDSQT(sol->model)

    int penalties = 0;

    FOR_C {
        int sum_w_cr = 0;

        FOR_R {
            int w_cr = 0;

            FOR_D {
                FOR_S {
                    w_cr = w_cr || solution_get(sol, c, r, d, s);
                }
            };

            sum_w_cr += w_cr;
        }

        if (sum_w_cr > 1) {
            debug2("S4 (RoomStability) penalty: course '%s' uses %d rooms",
                  sol->model->courses[c].id, sum_w_cr);
            if (strout)
                strappend_realloc(strout, strsize,
                    "S4 (RoomStability) penalty: course '%s' uses %d rooms\n",
                    sol->model->courses[c].id, sum_w_cr);

            penalties += sum_w_cr - 1;
        }
    };

    return penalties * ROOM_STABILITY_COST;
}

int solution_room_stability_cost(const solution *sol) {
    return solution_room_stability_cost_dump(sol, NULL, NULL);
}

char *solution_quality_to_string(const solution *sol, bool verbose) {
    char *str = NULL;
    size_t strsize;

    if (verbose)
        strappend_realloc(&str, &strsize, "%s", "");

    int h1 = solution_lectures_violations_dump(sol, &str, &strsize);
    int h2 = solution_room_occupancy_violations_dump(sol, &str, &strsize);
    int h3 = solution_conflicts_violations_dump(sol, &str, &strsize);
    int h4 = solution_availabilities_violations_dump(sol, &str, &strsize);
    int s1 = solution_room_capacity_cost_dump(sol, &str, &strsize);
    int s2 = solution_min_working_days_cost_dump(sol, &str, &strsize);
    int s3 = solution_curriculum_compactness_cost_dump(sol, &str, &strsize);
    int s4 = solution_room_stability_cost_dump(sol, &str, &strsize);

    if (!str)
        strappend_realloc(&str, &strsize, "");

    strappend_realloc(&str, &strsize,
        "HARD constraints satisfied: %s\n"
        "  Lectures violations: %d\n"
        "  RoomOccupancy violations: %d\n"
        "  Conflicts violations: %d\n"
        "  Availabilities violations: %d\n"
        "SOFT constraints cost: %d\n"
        "  RoomCapacity cost: %d\n"
        "  MinWorkingDays cost: %d\n"
        "  CurriculumCompactness cost: %d\n"
        "  RoomStability cost: %d",
        BOOL_TO_STR(!h1 && !h2 && !h3 && !h4),
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



void solution_parser_init(solution_parser *solution_parser) {
    solution_parser->error = NULL;
}

void solution_parser_destroy(solution_parser *solution_parser) {
    free(solution_parser->error);
}

bool solution_parser_parse(solution_parser *solution_parser,
                           const char *input, solution *sol) {

#define ABORT_PARSE(errfmt, ...) do { \
    snprintf(error_reason, MAX_ERROR_LENGTH, errfmt, ##__VA_ARGS__); \
    goto QUIT; \
} while(0)

#define ABORT_PARSE_INT_FAIL() ABORT_PARSE("integer conversion failed")

    static const int MAX_ERROR_LENGTH = 256;
    static const int MAX_LINE_LENGTH = 256;

    char error_reason[MAX_ERROR_LENGTH];
    error_reason[0] = '\0';
    int line_num = 0;

    char line0[MAX_LINE_LENGTH];
    char *line;

    verbose("Opening input file: '%s'", input);

    FILE *file = fopen(input, "r");
    if (!file)
        ABORT_PARSE("failed to open '%s' (%s)\n", input, strerror(errno));

    const model *m = sol->model;

    while (fgets(line0, LENGTH(line0), file)) {
        ++line_num;
        line = strtrim(line0);
        verbose("<< %s", line);

        if (strempty(line))
            continue;


        char *F[4];
        bool ok;
        int n_tokens = strsplit(line, " ", F, 4);
        if (n_tokens != 4)
            ABORT_PARSE("unexpected tokens count: found %d instead of 4", n_tokens);

        solution_set(
                sol,
                model_course_by_id(m, F[0])->index,
                model_room_by_id(m, F[1])->index,
                strtoint(F[2], &ok),
                strtoint(F[3], &ok), 1);

        if (!ok)
            ABORT_PARSE_INT_FAIL();
    }

    verbose("Closing file: '%s'", input);
    if (fclose(file) != 0)
        verbose("WARN: failed to close '%s' (%s)", input, strerror(errno));

QUIT:
    if (!strempty(error_reason)) {
        debug("Parser error: %s", error_reason);
        solution_parser->error = strmake("parse error at line %d (%s)", line_num, error_reason);
    }

    bool success = strempty(solution_parser->error);

#undef ABORT_PARSE
#undef ABORT_PARSE_INT_FAIL

    return success;
}

const char *solution_parser_get_error(solution_parser *solution_parser) {
    return solution_parser->error;
}

void solution_set(solution *sol, int c, int r, int d, int s, bool value) {
    sol->timetable[INDEX4(c, sol->model->n_courses, r, sol->model->n_rooms,
                          d, sol->model->n_days, s, sol->model->n_slots)] = value;
}

bool solution_get(const solution *sol, int c, int r, int d, int s) {
    return sol->timetable[INDEX4(c, sol->model->n_courses, r, sol->model->n_rooms,
                                 d, sol->model->n_days, s, sol->model->n_slots)];
}

solution_fingerprint_t solution_fingerprint(const solution *sol) {
    solution_fingerprint_t fingerprint = 0;
    for (int i = 0; i < sol->model->n_courses * sol->model->n_rooms * sol->model->n_days * sol->model->n_slots; i++) {
        if (sol->timetable[i]) {
//            debug("solution_fingerprint(%d)=%d", i, i);
//            fingerprint += g_int_hash(&i);
            fingerprint += i;
        }
//        fingerprint *= 13;
    }
    return fingerprint;
}

void solution_set_at(solution *sol, int index, bool value) {
    sol->timetable[index] = value;
}

bool solution_get_at(const solution *sol, int index) {
    return sol->timetable[index];
}

bool read_solution(solution *sol, const char *input_file) {
    if (!input_file)
        return false;

    solution_parser sp;
    solution_parser_init(&sp);
    bool success = solution_parser_parse(&sp, input_file, sol);
    if (!success) {
        eprint("ERROR: failed to parse solution file '%s' (%s)",
               input_file, solution_parser_get_error(&sp));
    }

    solution_parser_destroy(&sp);

    return success;
}

bool write_solution(const solution *sol, const char *output_file) {
    if (!output_file)
        return false;

    char *sol_str = solution_to_string(sol);

    bool success = filewrite(output_file, sol_str);
    if (!success)
        eprint("ERROR: failed to write output solution to '%s' (%s)",
               output_file, strerror(errno));

    free(sol_str);

    return success;
}

void print_solution(const solution *sol, FILE *stream) {
    char *sol_str = solution_to_string(sol);
    fprint(stream, "%s", sol_str);
    free(sol_str);
}

void print_solution_full(const solution *sol, FILE *stream) {
    char *sol_str = solution_to_string(sol);
    char *sol_quality_str = solution_quality_to_string(sol, true);

    fprint(stream,
           "%s\n"
          "----------------------\n"
          "%s",
          sol_str,
          sol_quality_str);

    free(sol_str);
    free(sol_quality_str);
}

int solution_assignment_count(const solution *sol) {
    CRDSQT(sol->model);
    int count = 0;
    for (int i = 0; i < C * R * D * S; i++)
        count += sol->timetable[i];
    return count;
}
