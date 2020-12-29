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

void course_to_string(const course *course, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
             "(id=%s, teacher=%s, n_lectures=%d, min_working_days=%d, n_students=%d)",
             course->id, course->teacher, course->n_lectures,
             course->min_working_days, course->n_students
    );
}

void room_to_string(const room *r, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
             "(id=%s, capacity=%d)",
             r->id, r->capacity
    );
}


void curricula_to_string(const curricula *q, char *buffer, size_t buflen) {
    char *courses_str = strjoin(q->courses, q->n_courses, ", ");

    snprintf(buffer, buflen,
             "(id=%s, n_courses=%d, courses=[%s])",
             q->id, q->n_courses, courses_str
    );

    free(courses_str);
}

void unavailability_constraint_to_string(const unavailability_constraint *uc,
                                         char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
             "(course=%s, day=%d, day_period=%d)",
             uc->course, uc->day, uc->day_period
    );
}

#define MODEL_APPEND_SECTION(header, n_entries, entries, to_string_func) do { \
    if (entries) { \
        strappend_realloc(&buffer, &buflen, header); \
        for (int i = 0; i < (n_entries); ++i) { \
            to_string_func(&(entries)[i], tmp, tmp_buffer_length); \
            strappend_realloc(&buffer, &buflen, "%s\n", tmp); \
        } \
    } \
} while(0)

char * model_to_string(const model *model) {
    size_t buflen = 256;
    char *buffer = mallocx(sizeof(char) * buflen);

    buffer[0] = '\0';
    strappend_realloc(&buffer, &buflen,
         "name = %s\n"
         "# courses = %d\n"
         "# rooms = %d\n"
         "# days = %d\n"
         "# periods_per_day = %d\n"
         "# curricula = %d\n"
         "# constraints = %d\n",
         model->name,
         model->n_courses,
         model->n_rooms,
         model->n_days,
         model->n_periods_per_day,
         model->n_curriculas,
         model->n_unavailability_constraints
    );

    static const size_t tmp_buffer_length = 512;
    char tmp[tmp_buffer_length];

    MODEL_APPEND_SECTION("COURSES\n",
                         model->n_courses, model->courses,
                         course_to_string);
    MODEL_APPEND_SECTION("ROOMS\n",
                         model->n_rooms, model->rooms,
                         room_to_string);
    MODEL_APPEND_SECTION("CURRICULAS\n",
                         model->n_curriculas, model->curriculas,
                         curricula_to_string);
    MODEL_APPEND_SECTION("UNAVAILABILITY_CONSTRAINTS\n",
                         model->n_unavailability_constraints,
                         model->unavailability_constraints,
                         unavailability_constraint_to_string);

    buffer[strlen(buffer) - 1] = '\0';


    return buffer; // must be freed outside
}
