#include "model.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void course_init(course *course) {
    course->id = NULL;
    course->teacher = NULL;
    course->n_lectures = 0;
    course->min_working_days = 0;
    course->n_students = 0;
}

void course_dump(const course *course, char *dump, int size, const char *indent) {
    snprintf(dump, size,
             "%s(id=%s, teacher=%s, n_lectures=%d, min_working_days=%d, n_students=%d)",
             indent,
             course->id, course->teacher, course->n_lectures,
             course->min_working_days, course->n_students
    );
}

void model_init(model *model) {
    model->name = NULL;
    model->n_courses = 0;
    model->n_rooms = 0;
    model->n_days = 0;
    model->n_periods_per_day = 0;
    model->n_curricula = 0;
    model->n_unavailability_constraints = 0;
    model->courses = NULL;
    model->rooms = NULL;
    model->curriculas = NULL;
    model->unavailability_constraints = NULL;
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
             indent, model->n_curricula,
             indent, model->n_unavailability_constraints
    );

    strappendf(dump, size, "%sCOURSES\n", indent);

    if (model->courses) {
        for (int i = 0; i < model->n_courses; ++i) {
            course_dump(&model->courses[i], &dump[strlen(dump)], (int) (size - strlen(dump)), two_indent);
            strappend(dump, size, "\n");
        }
    }
}

void model_destroy(model *model) {
    free(model->name);
    free(model->courses);
    free(model->rooms);
    free(model->curriculas);
    free(model->unavailability_constraints);
}