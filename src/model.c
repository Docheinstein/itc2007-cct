#include "model.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void model_init(model *model) {
    model->name = NULL;
    model->n_courses = 0;
    model->n_rooms = 0;
    model->n_days = 0;
    model->n_periods_per_day = 0;
    model->n_curriculas = 0;
    model->n_unavailability_constraints = 0;
    model->courses = NULL;
    model->rooms = NULL;
    model->curriculas = NULL;
    model->unavailability_constraints = NULL;
}

void course_destroy(const course *c) {
    free(c->id);
    free(c->teacher);
}

void room_destroy(const room *r) {
    free(r->id);
}

void curricula_destroy(const curricula *q) {
    free(q->id);
    for (int i = 0; i < q->n_courses; i++)
        free(q->courses[i]);
    free(q->courses);
}

void unavailability_constraint_destroy(const unavailability_constraint *uc) {
    free(uc->course);
}

void model_destroy(const model *model) {
    free(model->name);

    for (int i = 0; i < model->n_courses; i++)
        course_destroy(&model->courses[i]);
    free(model->courses);

    for (int i = 0; i < model->n_rooms; i++)
        room_destroy(&model->rooms[i]);
    free(model->rooms);

    for (int i = 0; i < model->n_curriculas; i++)
        curricula_destroy(&model->curriculas[i]);
    free(model->curriculas);

    for (int i = 0; i < model->n_unavailability_constraints; i++)
        unavailability_constraint_destroy(&model->unavailability_constraints[i]);
    free(model->unavailability_constraints);
}

void course_dump(const course *course, char *dump, int size, const char *indent) {
    snprintf(dump, size,
             "%s(id=%s, teacher=%s, n_lectures=%d, min_working_days=%d, n_students=%d)",
             indent,
             course->id, course->teacher, course->n_lectures,
             course->min_working_days, course->n_students
    );
}

void room_dump(const room *room, char *dump, int size, const char *indent) {
    snprintf(dump, size,
             "%s(id=%s, capacity=%d)",
             indent,
             room->id, room->capacity
    );
}


void curricula_dump(const curricula *q, char *dump, int size, const char *indent) {
    char *courses_str = strjoin(q->courses, q->n_courses, ", ");

    snprintf(dump, size,
             "%s(id=%s, n_courses=%d, courses=[%s])",
             indent,
             q->id, q->n_courses, courses_str
    );

    free(courses_str);
}

void unavailability_constraint_dump(const unavailability_constraint *uc,
                                    char *dump, int size, const char *indent) {
    snprintf(dump, size,
             "%s(course=%s, day=%d, day_period=%d)",
             indent,
             uc->course, uc->day, uc->day_period
    );
}

void model_dump(const model *model, char *dump, int size, const char *indent) {
    char two_indent[strlen(indent) * 2 + 1];
    snprintf(two_indent, strlen(indent) * 2 + 1, "%s%s", indent, indent);

    snprintf(dump, size,
             "%sname = %s\n"
             "%s# courses = %d\n"
             "%s# rooms = %d\n"
             "%s# days = %d\n"
             "%s# n_periods_per_day = %d\n"
             "%s# n_curricula = %d\n"
             "%s# n_constraints = %d\n",
             indent, model->name,
             indent, model->n_courses,
             indent, model->n_rooms,
             indent, model->n_days,
             indent, model->n_periods_per_day,
             indent, model->n_curriculas,
             indent, model->n_unavailability_constraints
    );

    if (model->courses) {
        strappendf(dump, size, "%sCOURSES\n", indent);

        for (int i = 0; i < model->n_courses; ++i) {
            course_dump(&model->courses[i], &dump[strlen(dump)],
                        (int) (size - strlen(dump)), two_indent);
            strappend(dump, size, "\n");
        }
    }

    if (model->rooms) {
        strappendf(dump, size, "%sROOMS\n", indent);

        for (int i = 0; i < model->n_rooms; ++i) {
            room_dump(&model->rooms[i], &dump[strlen(dump)],
                      (int) (size - strlen(dump)), two_indent);
            strappend(dump, size, "\n");
        }
    }

    if (model->curriculas) {
        strappendf(dump, size, "%sCURRICULAS\n", indent);

        for (int i = 0; i < model->n_curriculas; ++i) {
            curricula_dump(&model->curriculas[i], &dump[strlen(dump)],
                           (int) (size - strlen(dump)), two_indent);
            strappend(dump, size, "\n");
        }
    }

    if (model->unavailability_constraints) {
        strappendf(dump, size, "%sUNAVAILABILITY_CONSTRAINTS\n", indent);

        for (int i = 0; i < model->n_unavailability_constraints; ++i) {
            unavailability_constraint_dump(
                    &model->unavailability_constraints[i], &dump[strlen(dump)],
                    (int) (size - strlen(dump)), two_indent);
            strappend(dump, size, "\n");
        }
    }
    dump[strlen(dump) - 1] = '\0';
}
