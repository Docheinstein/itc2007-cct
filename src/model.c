#include "model.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <str_utils.h>
#include "mem_utils.h"
#include "array_utils.h"
#include "debug.h"

void model_init(model *model) {
    model->name = NULL;
    model->n_courses = 0;
    model->n_rooms = 0;
    model->n_days = 0;
    model->n_slots = 0;
    model->n_curriculas = 0;
    model->n_unavailability_constraints = 0;
    model->courses = NULL;
    model->rooms = NULL;
    model->curriculas = NULL;
    model->unavailability_constraints = NULL;

    model->course_belongs_to_curricula = NULL;
    model->teachers = NULL;
    model->n_teachers = 0;
    model->course_taught_by_teacher = NULL;
    model->course_availabilities = NULL;
}

void course_destroy(const course *c) {
    free(c->id);
    free(c->teacher_id);
}

void room_destroy(const room *r) {
    free(r->id);
}

void curricula_destroy(const curricula *q) {
    free(q->id);
    freearray(q->courses_ids, q->n_courses);
}

void unavailability_constraint_destroy(const unavailability_constraint *uc) {
    free(uc->course_id);
}

void teacher_destroy(const teacher *t) {
    free(t->id);
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

    for (int i = 0; i < model->n_teachers; i++)
        teacher_destroy(&model->teachers[i]);
    free(model->teachers);

    free(model->course_belongs_to_curricula);
    free(model->course_taught_by_teacher);
    free(model->course_availabilities);
}

void course_to_string(const course *course, char *buffer, size_t buflen) {
    snprintf(buffer, buflen,
             "(id=%s, teacher=%s, n_lectures=%d, min_working_days=%d, n_students=%d)",
             course->id, course->teacher_id, course->n_lectures,
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
    char *courses_str = strjoin(q->courses_ids, q->n_courses, ", ");

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
             uc->course_id, uc->day, uc->slot
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
    char *buffer = NULL;
    size_t buflen;

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
         model->n_slots,
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

void course_set(course *c,
                char *id, char *teacher_id, int n_lectures,
                int min_working_days, int n_students) {
    c->id = id;
    c->teacher_id = teacher_id;
    c->n_lectures = n_lectures;
    c->min_working_days = min_working_days;
    c->n_students = n_students;
}

void room_set(room *r,
              char *id, int capacity) {
    r->id = id;
    r->capacity = capacity;
}

void curricula_set(curricula *q,
                   char *id, int n_courses, char **courses_ids) {
    q->id = id;
    q->n_courses = n_courses;
    q->courses_ids = courses_ids;
}

void unavailability_constraint_set(unavailability_constraint *uc,
                                   char *course_id, int day, int day_period) {
    uc->course_id = course_id;
    uc->day = day;
    uc->slot = day_period;
}

void model_finalize(model *model) {
    // Compute redundant data (for faster access)
    const int C = model->n_courses;
    const int R = model->n_rooms;
    const int Q = model->n_curriculas;
    const int U = model->n_unavailability_constraints;
    const int D = model->n_days;
    const int S = model->n_slots;

    static const int TMP_LEN = 256;
    char tmp[TMP_LEN];

    // l_c
//    model->course_lectures = mallocx(sizeof(int) * C);
//    for (int c = 0; c < C; c++) {
//        model->course_lectures[c] = model->courses[c].n_lectures;
//    }

    // b_cq
    model->course_belongs_to_curricula = mallocx(sizeof(bool) * Q * C);
    for (int q = 0; q < Q; q++) {
        const curricula *curricula = &model->curriculas[q];

        for (int c = 0; c < C; c++) {
            const course *course = &model->courses[c];
            model->course_belongs_to_curricula[INDEX2(q, Q, c, C)] = 0;

            for (int qc = 0; qc < curricula->n_courses; qc++) {
                if (streq(course->id, curricula->courses_ids[qc])) {
                    model->course_belongs_to_curricula[INDEX2(q, Q, c, C)] = 1;
                    break;
                }
            }
        }
    }

    // T
    GHashTable *teachers_set = g_hash_table_new(g_str_hash, g_str_equal);

    for (int c = 0; c < C; c++)
        g_hash_table_add(teachers_set, model->courses[c].teacher_id);

    uint T = g_hash_table_size(teachers_set);
    model->teachers = mallocx(sizeof(teacher) * T);
    model->n_teachers = T;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, teachers_set);
    int i = 0;
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        model->teachers->index = i;
        model->teachers[i++].id = strdup(key);
    }

    g_hash_table_destroy(teachers_set);

    // e_ct
    model->course_taught_by_teacher = mallocx(sizeof(bool) * C * T);
    for (int c = 0; c < C; c++) {
        const char *course_teacher = model->courses[c].teacher_id;
        for (int t = 0; t < T; t++) {
            model->course_taught_by_teacher[INDEX2(c, C, t, T)] =
                    streq(course_teacher, model->teachers[t].id);
        }
    }

    // a_cds

    GHashTable *unavailabilities_set = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
//    GHashTable *unavailabilities_set = g_hash_table_new(g_str_hash, g_str_equal);

    for (int u = 0; u < U; u++) {
        const struct unavailability_constraint *uc = &model->unavailability_constraints[u];
        g_hash_table_add(unavailabilities_set, strmake("%s_%d_%d", uc->course_id, uc->day, uc->slot));
    }

    model->course_availabilities = mallocx(sizeof(bool) * C * D * S);
    for (int c = 0; c < C; c++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                snprintf(tmp, TMP_LEN, "%s_%d_%d", model->courses[c].id, d, s);
                model->course_availabilities[INDEX3(c, C, d, D, s, S)] =
                        !g_hash_table_contains(unavailabilities_set, tmp);
            }
        }
    }

    g_hash_table_destroy(unavailabilities_set);
}

course *model_course_by_id(const model *model, char *id) {
    for (int i = 0; i < model->n_courses; i++) {
        course *c = &model->courses[i];
        if (streq(id, c->id))
            return c;
    }
    return NULL;
}

room *model_room_by_id(const model *model, char *id) {
    for (int i = 0; i < model->n_rooms; i++) {
        room *r = &model->rooms[i];
        if (streq(id, r->id))
            return r;
    }
    return NULL;
}

curricula *model_curricula_by_id(const model *model, char *id) {
    for (int i = 0; i < model->n_curriculas; i++) {
        curricula *q = &model->curriculas[i];
        if (streq(id, q->id))
            return q;
    }
    return NULL;
}

int model_course_belongs_to_curricula(const model *model, const course *course, const curricula *curricula) {
    return model_course_belongs_to_curricula_by_index(model, course->index, curricula->index);
}

int model_course_taught_by_teacher(const model *model, const course *course, const teacher *teacher) {
    return model_course_taught_by_teacher_by_index(model, course->index, teacher->index);
}

int model_course_is_available_on_period(const model *model, const course *course, int day, int slot) {
    return model_course_is_available_on_period_by_index(model, course->index, day, slot);
}

int model_course_belongs_to_curricula_by_index(const model *model, int course_idx, int curricula_idx) {
    return model->course_belongs_to_curricula[
            INDEX2(curricula_idx, model->n_curriculas, course_idx, model->n_courses)];
}

int model_course_taught_by_teacher_by_index(const model *model, int course_idx, int teacher_idx) {
    return model->course_taught_by_teacher[
            INDEX2(course_idx, model->n_courses, teacher_idx, model->n_teachers)];
}

int model_course_is_available_on_period_by_index(const model *model, int course_idx, int day, int slot) {
    return model->course_availabilities[
            INDEX3(course_idx, model->n_courses, day, model->n_days, slot, model->n_slots)];
}

