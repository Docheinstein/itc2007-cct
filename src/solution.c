#include <utils/str_utils.h>
#include <utils/io_utils.h>
#include "solution.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "log/debug.h"
#include "config.h"
#include "log/verbose.h"
#include "utils/def_utils.h"

#define FOR_C for (int c = 0; c < sol->model->n_courses; c++)
#define FOR_R for (int r = 0; r < sol->model->n_rooms; r++)
#define FOR_D for (int d = 0; d < sol->model->n_days; d++)
#define FOR_S for (int s = 0; s < sol->model->n_slots; s++)
#define FOR_Q for (int q = 0; q < sol->model->n_curriculas; q++)
#define FOR_T for (int t = 0; t < sol->model->n_teachers; t++)

void solution_init(solution *sol, const model *model) {
    sol->timetable = callocx(
            model->n_courses * model->n_rooms * model->n_days * model->n_slots,
            sizeof(bool));
    sol->model = model;
}

void solution_destroy(solution *sol) {
    free(sol->timetable);
}


void solution_copy(solution *solution_dest, const solution *solution_src) {
    memcpy(solution_dest->timetable, solution_src->timetable,
           solution_dest->model->n_courses * solution_dest->model->n_rooms *
           solution_dest->model->n_days * solution_dest->model->n_slots * sizeof(bool));
    solution_dest->model = solution_src->model; // should already be the same
}

char * solution_to_string(const solution *sol) {
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

char *solution_quality_to_string(solution *sol) {
    int h1 = solution_hard_constraint_lectures_violations(sol);
    int h2 = solution_hard_constraint_room_occupancy_violations(sol);
    int h3 = solution_hard_constraint_conflicts_violations(sol);
    int h4 = solution_hard_constraint_availabilities_violations(sol);
    int s1 = solution_soft_constraint_room_capacity(sol);
    int s2 = solution_soft_constraint_min_working_days(sol);
    int s3 = solution_soft_constraint_curriculum_compactness(sol);
    int s4 = solution_soft_constraint_room_stability(sol);

    return strmake(
            "HARD constraints satisfied: %s\n"
            "\tLectures violations: %d\n"
            "\tRoomOccupancy violations: %d\n"
            "\tConflicts violations: %d\n"
            "\tAvailabilities violations: %d\n"
            "SOFT constraints cost: %d\n"
            "\tRoomCapacity cost: %d\n"
            "\tMinWorkingDays cost: %d\n"
            "\tCurriculumCompactness cost: %d\n"
            "\tRoomStability cost: %d",
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
}


bool solution_satisfy_hard_constraints(const solution *sol) {
    return
        solution_satisfy_hard_constraint_lectures(sol) &&
        solution_satisfy_hard_constraint_room_occupancy(sol) &&
        solution_satisfy_hard_constraint_conflicts(sol) &&
        solution_satisfy_hard_constraint_availabilities(sol);
}

bool solution_satisfy_hard_constraint_lectures(const solution *sol) {
    return solution_hard_constraint_lectures_violations(sol) == 0;
}

bool solution_satisfy_hard_constraint_room_occupancy(const solution *sol) {
    return solution_hard_constraint_room_occupancy_violations(sol) == 0;
}


bool solution_satisfy_hard_constraint_availabilities(const solution *sol) {
    return solution_hard_constraint_availabilities_violations(sol) == 0;
}

bool solution_satisfy_hard_constraint_conflicts(const solution *sol) {
    return solution_hard_constraint_conflicts_violations(sol) == 0;
}


int solution_hard_constraint_lectures_violations(const solution *sol) {
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
            violations++;
        }
    }

    FOR_C {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += solution_get(sol, c, r, d, s);
                }
                if (!(n <= 1)) {
                    debug2("H1 (Lectures) violation: %d rooms used for course '%s' "
                          "on (day=%d, slot=%d)",
                          n, sol->model->courses[c].id, d, s);
                    violations++;
                }
            }
        }
    }

    return violations;
}

int solution_hard_constraint_room_occupancy_violations(const solution *sol) {
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
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_hard_constraint_conflicts_violations(const solution *sol) {
    int violations = 0;

    FOR_Q {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_C {
                    FOR_R {
                        if (model_course_belongs_to_curricula(sol->model, c, q))
                            n += solution_get(sol, c, r, d, s);
                    }
                };

                if (!(n <= 1)) {
                    debug2("H3 (Conflicts) violation: %d courses of curriculum '%s' "
                          "scheduled on (day=%d, slot=%d)",
                          n, sol->model->curriculas[q].id, d, s);
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
                        if (model_course_is_taught_by_teacher(sol->model, c, t))
                            n += solution_get(sol, c, r, d, s);
                    }
                };

                if (!(n <= 1)) {
                    debug2("H3 (Conflicts) violation: %d courses taught by "
                          "teacher '%s' scheduled on (day=%d, slot=%d)",
                          n, sol->model->teachers[t].id, d, s);
                    violations++;
                }
            }
        }
    }



    return violations;
}

int solution_hard_constraint_availabilities_violations(const solution *sol) {
    int violations = 0;

    FOR_C {
        FOR_D {
            FOR_S {
                int n = 0;
                FOR_R {
                    n += solution_get(sol, c, r, d, s);
                }
                if (!(n <= model_course_is_available_on_period(sol->model, c, d, s))) {
                    debug2("H4 (Availabilities) violation: course '%s' scheduled %d times on "
                          "(day=%d, slot=%d) but breaks unavailability constraint",
                          sol->model->courses[c].id, n, d, s);
                    violations++;
                }
            };
        }
    }

    return violations;
}

int solution_cost(const solution *sol) {
    return
        solution_soft_constraint_room_capacity(sol) +
        solution_soft_constraint_min_working_days(sol) +
        solution_soft_constraint_curriculum_compactness(sol) +
        solution_soft_constraint_room_stability(sol);
}



int solution_soft_constraint_room_capacity(const solution *sol) {
    int penalties = 0;

    FOR_C {
        FOR_R {
            FOR_D {
                FOR_S {
                    if (solution_get(sol, c, r, d, s)) {
                        int delta = sol->model->courses[c].n_students > sol->model->rooms[r].capacity;
                        if (delta > 0) {
                            debug2("S1 (RoomCapacity) penalty: course '%s' has %d"
                                  " students but it's scheduled in room '%s' with %d "
                                  "seats on (day=%d, slot=%d)",
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

int solution_soft_constraint_min_working_days(const solution *sol) {
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
            penalties++;
        }
    };

    return penalties * MIN_WORKING_DAYS_COST;
}

int solution_soft_constraint_curriculum_compactness(const solution *sol) {
    int penalties = 0;

    bool * slots = mallocx(sol->model->n_slots, sizeof(bool));

    FOR_Q {
        FOR_D {
            FOR_S {

                int z_qds = 0;

                FOR_C {
                    FOR_R {
                        z_qds = z_qds ||
                                (solution_get(sol, c, r, d, s) *
                                 model_course_belongs_to_curricula(sol->model, c, q));
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
                    penalties++;
                }
            }
        }
    };

    free(slots);
    return penalties * CURRICULUM_COMPACTNESS_COST;
}

int solution_soft_constraint_room_stability(const solution *sol) {
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

unsigned long long solution_fingerprint(const solution *sol) {
    unsigned long long fingerprint = 0;
    for (int i = 0; i < sol->model->n_courses * sol->model->n_rooms * sol->model->n_days * sol->model->n_slots; i++) {
        if (sol->timetable[i])
            fingerprint++;
        fingerprint *= 23;
    }
    return fingerprint;
}

void solution_set_at(solution *sol, int index, bool value) {
    sol->timetable[index] = value;
}

bool solution_get_at(const solution *sol, int index) {
    return sol->timetable[index];
}

