#include <str_utils.h>
#include "solution.h"
#include "mem_utils.h"
#include "array_utils.h"
#include "debug.h"
#include "io_utils.h"
#include "config.h"
#include "verbose.h"

#define FOR_C \
    const int C = model->n_courses; \
    for (int c = 0; c < C; c++)

#define FOR_C2 \
    for (int c = 0; c < C; c++)

#define FOR_R \
    const int R = model->n_rooms; \
    for (int r = 0; r < R; r++)

#define FOR_R2 \
    for (int r = 0; r < R; r++)

#define FOR_D \
    const int D = model->n_days; \
    for (int d = 0; d < model->n_days; d++)

#define FOR_D2 \
    for (int d = 0; d < model->n_days; d++)

#define FOR_S \
    const int S = model->n_slots; \
    for (int s = 0; s < model->n_slots; s++)

#define FOR_S2 \
    for (int s = 0; s < model->n_slots; s++)

#define FOR_Q \
    const int Q = model->n_curriculas; \
    for (int q = 0; q < Q; q++)

#define FOR_Q2 \
    for (int q = 0; q < Q; q++)

#define FOR_T \
    const int T = model->n_teachers; \
    for (int t = 0; t < Q; t++)

#define FOR_T2 \
    for (int t = 0; t < Q; t++)

assignment *assignment_new(course *course, room *room, int day, int day_period) {
    assignment *a = mallocx(sizeof(assignment));
    assignment_set(a, course, room, day, day_period);
    return a;
}

void assignment_set(assignment *a, course *course, room *room, int day, int day_period) {
    a->course = course;
    a->room = room;
    a->day = day;
    a->slot = day_period;
}

void solution_init(solution *sol) {
    sol->assignments = NULL;
}


void solution_finalize(solution *sol, const model *model) {
    debug("Computing timetable...");

    sol->timetable = callocx(
        model->n_courses * model->n_rooms * model->n_days * model->n_slots,
        sizeof(bool)
    );

    for (GList *p = sol->assignments; p != NULL; p = p->next) {
        const assignment *a = p->data;
        debug("X[%s][%s][%d][%d] = 1", a->course->id, a->room->id, a->day, a->slot);
        sol->timetable[INDEX4(
                a->course->index, model->n_courses,
                a->room->index, model->n_rooms,
                a->day, model->n_days,
                a->slot, model->n_slots
        )] = true;
    }

}

void solution_destroy(solution *sol) {
    g_list_free_full(sol->assignments, free);
    free(sol->timetable);
}

char * solution_to_string(const solution *sol) {
    char *buffer = NULL;
    size_t buflen;

    for (GList *p = sol->assignments; p != NULL; p = p->next) {
        const assignment *a = p->data;
        strappend_realloc(&buffer, &buflen, "%s %s %d %d\n",
                          a->course->id, a->room->id, a->day, a->slot);
    }

    buffer[strlen(buffer) - 1] = '\0';
    return buffer;
}

void solution_add_assignment(solution *sol, assignment *a) {
    sol->assignments = g_list_append(sol->assignments, a);
}

bool solution_satisfy_hard_constraints(solution *sol, const model *model) {
    return
        solution_satisfy_hard_constraint_lectures(sol, model) &&
        solution_satisfy_hard_constraint_room_occupancy(sol, model) &&
        solution_satisfy_hard_constraint_conflicts(sol, model) &&
        solution_satisfy_hard_constraint_availabilities(sol, model);
}

bool solution_satisfy_hard_constraint_lectures(solution *sol, const model *model) {
    return solution_hard_constraint_lectures_violations(sol, model) == 0;
}

bool solution_satisfy_hard_constraint_room_occupancy(solution *sol, const model *model) {
    return solution_hard_constraint_room_occupancy_violations(sol, model) == 0;
}


bool solution_satisfy_hard_constraint_availabilities(solution *sol, const model *model) {
    return solution_hard_constraint_availabilities_violations(sol, model) == 0;
}

bool solution_satisfy_hard_constraint_conflicts(solution *sol, const model *model) {
    return solution_hard_constraint_conflicts_violations(sol, model) == 0;
}


int solution_hard_constraint_lectures_violations(solution *sol, const model *model) {
    int violations = 0;

    FOR_C {
        int n = 0;
        FOR_R {
            FOR_D {
                FOR_S {
                    n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }
        }
        const course *course = &model->courses[c];
        if (n != course->n_lectures) {
            debug("H1 (Lectures) violation: %d lectures assigned instead of %d "
                  "for course '%s'",
                  n, course->n_lectures, course->id);
            violations++;
        }
    }

    FOR_C2 {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
                if (!(n <= 1)) {
                    debug("H1 (Lectures) violation: %d rooms used for course '%s' "
                          "on (day=%d, slot=%d)",
                          n, model->courses[c].id, d, s);
                    violations++;
                }
            }
        }
    }

    return violations;
}

int solution_hard_constraint_room_occupancy_violations(solution *sol, const model *model) {
    int violations = 0;

    FOR_R {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
                if (!(n <= 1)) {
                    debug("H2 (RoomOccupancy) violation: %d courses in room '%s' "
                          "on (day=%d, slot=%d)",
                          n, model->rooms[r].id, d, s);
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_hard_constraint_conflicts_violations(solution *sol, const model *model) {
    int violations = 0;

    FOR_Q {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    FOR_R {
                        if (model_course_belongs_to_curricula_by_index(model, c, q))
                            n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                    }
                };

                if (!(n <= 1)) {
                    debug("H3 (Conflicts) violation: %d courses of curriculum '%s' "
                          "scheduled on (day=%d, slot=%d)",
                          n, model->curriculas[q].id, d, s);
                    violations++;
                }
            }
        }
    }

    FOR_T {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    FOR_R {
                        if (model_course_taught_by_teacher_by_index(model, c, t))
                            n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                    }
                };

                if (!(n <= 1)) {
                    debug("H3 (Conflicts) violation: %d courses taught by "
                          "teacher '%s' scheduled on (day=%d, slot=%d)",
                          n, model->teachers[t].id, d, s);
                    violations++;
                }
            }
        }
    }



    return violations;
}

int solution_hard_constraint_availabilities_violations(solution *sol, const model *model) {
    int violations = 0;

    FOR_C {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
                if (!(n <= model_course_is_available_on_period_by_index(model, c, d, s))) {
                    debug("H4 (Availabilities) violation: course '%s' scheduled %d times on "
                          "(day=%d, slot=%d) but breaks unavailability constraint",
                          model->courses[c].id, n, d, s);
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_cost(solution *sol, const model *model) {
    return
        solution_soft_constraint_room_capacity(sol, model) +
        solution_soft_constraint_min_working_days(sol, model) +
        solution_soft_constraint_curriculum_compactness(sol, model) +
        solution_soft_constraint_room_stability(sol, model);
}

int solution_soft_constraint_room_capacity(solution *sol, const model *model) {
    int penalties = 0;

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (sol->timetable[INDEX4(c, C, r, R, d, D, s, S)]) {
                        int delta = model->courses[c].n_students > model->rooms[r].capacity;
                        if (delta > 0) {
                            debug("S1 (RoomCapacity) penalty: course '%s' has %d"
                                  " students but it's scheduled on room '%s' with %d "
                                  "seats on (day=%d, slot=%d)",
                                  model->courses[c].id, model->courses[c].n_students,
                                  model->rooms[r].id, model->rooms[r].capacity,
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

int solution_soft_constraint_min_working_days(solution *sol, const model *model) {
    int penalties = 0;

    FOR_C {
        int sum_y_cd = 0;

        FOR_D {
            int y_cd = 0;

            FOR_R {
                FOR_S {
                    y_cd = y_cd || sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
            }

            sum_y_cd += y_cd;
        }

        int delta = model->courses[c].min_working_days - sum_y_cd;
        if (delta > 0) {
            debug("S2 (MinWorkingDays) penalty: course '%s' spread among %d different days "
                  "but Minimum Working Days is %d",
                  model->courses[c].id, sum_y_cd, model->courses[c].min_working_days);
            penalties++;
        }
    };

    return penalties * MIN_WORKING_DAYS_COST;
}

int solution_soft_constraint_curriculum_compactness(solution *sol, const model *model) {
    int penalties = 0;

    bool * slots = mallocx(sizeof(bool) * model->n_slots);

    FOR_Q {
        FOR_D {
            FOR_S {

                int z_qds = 0;

                FOR_C {
                    FOR_R {
                        z_qds = z_qds ||
                                (sol->timetable[INDEX4(c, C, r, R, d, D, s, S)] *
                                model_course_belongs_to_curricula_by_index(model, c, q));
                    }
                }

                slots[s] = z_qds;
            }

            FOR_S2 {
                bool prev = s > 0 && slots[s - 1];
                bool cur = slots[s];
                bool next = s < S - 1 && slots[s + 1];

                if (cur && !prev && !next) {
                    debug("S3 (CurriculumCompactness) penalty: curriculum '%s' "
                          "has a course scheduled alone on (day=%d, slot=%d) without adjacent lessons",
                          model->curriculas[q].id, d, s);
                    penalties++;
                }
            }
        }
    };

    free(slots);
    return penalties * CURRICULUM_COMPACTNESS_COST;
}

int solution_soft_constraint_room_stability(solution *sol, const model *model) {
    int penalties = 0;

    FOR_C {
        int sum_w_cr = 0;

        FOR_R {
            int w_cr = 0;

            FOR_D {
                FOR_S {
                    w_cr = w_cr || sol->timetable[INDEX4(c, C, r, R, d, D, s, S)];
                }
            };

            sum_w_cr += w_cr;
        }

        if (sum_w_cr > 1) {
            debug("S4 (RoomStability) penalty: course '%s' uses %d rooms",
                  model->courses[c].id, sum_w_cr);
            penalties++;
        }
    };

    return penalties * ROOM_STABILITY_COST;
}

void solution_parser_init(solution_parser *solution_parser) {
    solution_parser->error = NULL;
}

void solution_parser_destroy(solution_parser *solution_parser) {
    free(solution_parser->error);
}

bool solution_parser_parse(solution_parser *solution_parser, const model *model,
                           const char *input, solution *solution) {

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

        solution_add_assignment(solution,
            assignment_new(
                    model_course_by_id(model, F[0]),
                    model_room_by_id(model, F[1]),
                    strtoint(F[2], &ok),
                    strtoint(F[3], &ok
            )
        ));

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
    if (success)
        solution_finalize(solution, model);

#undef ABORT_PARSE
#undef ABORT_PARSE_INT_FAIL

    return success;
}

const char *solution_parser_get_error(solution_parser *solution_parser) {
    return solution_parser->error;
}
