#include "model.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <utils/str_utils.h>
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "log/debug.h"

static void course_destroy(const course *c) {
    free(c->id);
    free(c->teacher_id);
}

static void room_destroy(const room *r) {
    free(r->id);
}

static void curricula_destroy(const curricula *q) {
    free(q->id);
    freearray(q->courses_ids, q->n_courses);
}

static void unavailability_constraint_destroy(const unavailability_constraint *uc) {
    free(uc->course_id);
}

static void teacher_destroy(const teacher *t) {
    free(t->id);
}

void model_init(model *model) {
    static int model_id = 0;
    model->_id = model_id++;

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

    model->n_lectures = 0;
    model->lectures = NULL;
    model->n_teachers = 0;
    model->teachers = NULL;
    model->course_by_id = NULL;
    model->room_by_id = NULL;
    model->curricula_by_id = NULL;
    model->teacher_by_id = NULL;
    model->course_belongs_to_curricula = NULL;
    model->curriculas_of_course = NULL;
    model->courses_of_curricula = NULL;
    model->courses_of_teacher = NULL;
    model->course_taught_by_teacher = NULL;
    model->course_availabilities = NULL;
    model->courses_share_curricula = NULL;
    model->courses_same_teacher = NULL;
}

void model_destroy(const model *model) {
    debug("Destroying model '%s'", model->name);

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

    free(model->lectures);

    for (int i = 0; i < model->n_teachers; i++)
        teacher_destroy(&model->teachers[i]);
    free(model->teachers);

    if (model->curriculas_of_course) {
        for (int c = 0; c < model->n_courses; c++)
            g_array_free(model->curriculas_of_course[c], true);
        free(model->curriculas_of_course);
    }

    if (model->courses_of_teacher) {
        for (int t = 0; t < model->n_teachers; t++)
            g_array_free(model->courses_of_teacher[t], true);
        free(model->courses_of_teacher);
    }

    if (model->courses_of_curricula) {
        for (int q = 0; q < model->n_curriculas; q++)
            free(model->courses_of_curricula[q]);
        free(model->courses_of_curricula);
    }

    free(model->course_belongs_to_curricula);
    free(model->course_taught_by_teacher);
    free(model->course_availabilities);
    free(model->courses_share_curricula);
    free(model->courses_same_teacher);

    if (model->course_by_id)
        g_hash_table_destroy(model->course_by_id);
    if (model->room_by_id)
        g_hash_table_destroy(model->room_by_id);
    if (model->curricula_by_id)
        g_hash_table_destroy(model->curricula_by_id);
    if (model->teacher_by_id)
        g_hash_table_destroy(model->teacher_by_id);
}

void model_finalize(model *model) {
    // Compute redundant/implicit data (for faster access)

    debug("Finalizing model '%s'", model->name);

    const int C = model->n_courses;
    const int R = model->n_rooms;
    const int Q = model->n_curriculas;
    const int U = model->n_unavailability_constraints;
    const int D = model->n_days;
    const int S = model->n_slots;
    const int L = model->n_lectures;

    // model->teachers
    // model->n_teachers
    GHashTable *teachers_set = g_hash_table_new(g_str_hash, g_str_equal);

    for (int c = 0; c < C; c++)
        g_hash_table_add(teachers_set, model->courses[c].teacher_id);

    const uint T = g_hash_table_size(teachers_set);
    model->teachers = mallocx(T, sizeof(teacher));
    model->n_teachers = T;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, teachers_set);
    int i = 0;
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        model->teachers[i].index = i;
        model->teachers[i++].id = strdup(key);
    }

    g_hash_table_destroy(teachers_set);

    // model->course_by_id
    model->course_by_id = g_hash_table_new(g_str_hash, g_str_equal);
    for (int c = 0; c < C; c++)
        g_hash_table_insert(model->course_by_id, model->courses[c].id, &model->courses[c]);

    // model->room_by_id
    model->room_by_id = g_hash_table_new(g_str_hash, g_str_equal);
    for (int r = 0; r < R; r++)
        g_hash_table_insert(model->room_by_id, model->rooms[r].id, &model->rooms[r]);

    // model->curricula_by_id
    model->curricula_by_id = g_hash_table_new(g_str_hash, g_str_equal);
    for (int q = 0; q < Q; q++)
        g_hash_table_insert(model->curricula_by_id, model->curriculas[q].id, &model->curriculas[q]);

    // model->teacher_by_id
    model->teacher_by_id = g_hash_table_new(g_str_hash, g_str_equal);
    for (int t = 0; t < T; t++)
        g_hash_table_insert(model->teacher_by_id, model->teachers[t].id, &model->teachers[t]);

    // model->curriculas_of_course
    model->curriculas_of_course = mallocx(C, sizeof(GArray *));
    for (int c = 0; c < C; c++) {
        model->curriculas_of_course[c] = g_array_new(false, false, sizeof(int));
    }

    // model->course_belongs_to_curricula
    model->course_belongs_to_curricula = mallocx(Q * C, sizeof(bool));
    for (int q = 0; q < Q; q++) {
        const curricula *curricula = &model->curriculas[q];

        for (int c = 0; c < C; c++) {
            const course *course = &model->courses[c];
            model->course_belongs_to_curricula[INDEX2(q, Q, c, C)] = 0;

            for (int qc = 0; qc < curricula->n_courses; qc++) {
                if (streq(course->id, curricula->courses_ids[qc])) {
                    model->course_belongs_to_curricula[INDEX2(q, Q, c, C)] = 1;
                    g_array_append_val(model->curriculas_of_course[c], q);
                    break;
                }
            }
        }
    }

    // model->course_taught_by_teacher
    model->course_taught_by_teacher = mallocx(C * T, sizeof(bool));
    for (int c = 0; c < C; c++) {
        const char *course_teacher = model->courses[c].teacher_id;
        for (int t = 0; t < T; t++) {
            model->course_taught_by_teacher[INDEX2(c, C, t, T)] =
                    streq(course_teacher, model->teachers[t].id);
        }
    }

    // model->course_availabilities
    model->course_availabilities = mallocx(C * D * S, sizeof(bool));
    for (int x = 0; x < C * D * S; x++)
        model->course_availabilities[x] = true;
    for (int u = 0; u < U; u++) {
        unavailability_constraint *uc = &model->unavailability_constraints[u];
        uc->course = model_course_by_id(model, uc->course_id);
        model->course_availabilities[INDEX3(uc->course->index, C, uc->day, D, uc->slot, S)] = false;
    }

    // model->courses_of_teacher
    model->courses_of_teacher = mallocx(T, sizeof(GArray *));
    for (int t = 0; t < T; t++) {
        model->courses_of_teacher[t] = g_array_new(false, false, sizeof(int));
    }

    for (int t = 0; t < T; t++) {
        const teacher *teacher = &model->teachers[t];
        for (int c = 0; c < C; c++) {
            const course *course = &model->courses[c];
            if (streq(teacher->id, course->teacher_id)) {
                g_array_append_val(model->courses_of_teacher[t], c);
            }
        }
    }

    // model->courses_of_curricula
    model->courses_of_curricula = mallocx(Q, sizeof(int *));
    for (int q = 0; q < Q; q++) {
        const curricula *curricula = &model->curriculas[q];
        model->courses_of_curricula[q] = mallocx(curricula->n_courses, sizeof(int));

        for (int qc = 0; qc < curricula->n_courses; qc++) {
            model->courses_of_curricula[q][qc] = model_course_by_id(model, curricula->courses_ids[qc])->index;
        }
    }

    // model->courses_share_curricula
    model->courses_share_curricula = mallocx(C * C * Q, sizeof(bool));
    for (int c1 = 0; c1 < C; c1++) {
        for (int c2 = 0; c2 < C; c2++) {
            int c1_n_curriculas, c2_n_curriculas;
            int *c1_curriculas = model_curriculas_of_course(model, c1, &c1_n_curriculas);
            int *c2_curriculas = model_curriculas_of_course(model, c2, &c2_n_curriculas);
            for (int q = 0; q < Q; q++) {
                bool in_c1 = false;
                bool in_c2 = false;

                for (int c1_qi = 0; c1_qi < c1_n_curriculas && !in_c1; c1_qi++)
                    in_c1 = q == c1_curriculas[c1_qi];

                for (int c2_qi = 0; c2_qi < c2_n_curriculas && !in_c2; c2_qi++)
                    in_c2 = q == c2_curriculas[c2_qi];

                model->courses_share_curricula[INDEX3(c1, C, c2, C, q, Q)] = in_c1 && in_c2;
            }
        }
    }

    // model->courses_same_teacher
    model->courses_same_teacher = mallocx(C * C, sizeof(bool));
    for (int c1 = 0; c1 < C; c1++) {
        for (int c2 = 0; c2 < C; c2++) {
            model->courses_same_teacher[INDEX2(c1, C, c2, C)] =
                    streq(model->courses[c1].teacher_id,
                          model->courses[c2].teacher_id);
        }
    }

    // model->courses[c].teacher
    for (int c = 0; c < C; c++) {
        model->courses[c].teacher =
                model_teacher_by_id(model, model->courses[c].teacher_id);
    }

    // model->n_lectures
    // model->lectures
    for (int c = 0; c < C; c++)
        model->n_lectures += model->courses[c].n_lectures;

    int l = 0;
    model->lectures = mallocx(model->n_lectures, sizeof(lecture));
    for (int c = 0; c < C; c++) {
        const course *course = &model->courses[c];
        for (int n = 0; n < course->n_lectures; n++) {
            model->lectures[l].index = l;
            model->lectures[l].course = course;
            l++;
        }
    }
}

course *model_course_by_id(const model *model, char *id) {
    return g_hash_table_lookup(model->course_by_id, id);
}

room *model_room_by_id(const model *model, char *id) {
    return g_hash_table_lookup(model->room_by_id, id);
}

curricula *model_curricula_by_id(const model *model, char *id) {
    return g_hash_table_lookup(model->curricula_by_id, id);
}

teacher *model_teacher_by_id(const model *model, char *id) {
    return g_hash_table_lookup(model->teacher_by_id, id);
}

bool model_course_belongs_to_curricula(const model *model, int c, int q) {
    return model->course_belongs_to_curricula[
            INDEX2(q, model->n_curriculas, c, model->n_courses)];
}

bool model_course_is_taught_by_teacher(const model *model, int c, int t) {
    return model->course_taught_by_teacher[
            INDEX2(c, model->n_courses, t, model->n_teachers)];
}

bool model_course_is_available_on_period(const model *model, int c, int d, int s) {
    return model->course_availabilities[
            INDEX3(c, model->n_courses, d, model->n_days, s, model->n_slots)];
}

int *model_curriculas_of_course(const model *model, int c, int *n_curriculas) {
    if (n_curriculas)
        *n_curriculas = model->curriculas_of_course[c]->len;
    return (int *) model->curriculas_of_course[c]->data;
}

int *model_courses_of_curricula(const model *model, int q, int *n_courses) {
    if (n_courses)
        *n_courses = model->curriculas[q].n_courses;
    return model->courses_of_curricula[q];
}

int *model_courses_of_teacher(const model *model, int t, int *n_courses) {
    if (n_courses)
        *n_courses = model->courses_of_teacher[t]->len;
    return (int *) model->courses_of_teacher[t]->data;
}

bool model_share_curricula(const model *model, int c1, int c2, int q) {
    return model->courses_share_curricula[INDEX3(c1, model->n_courses,
                                                 c2, model->n_courses,
                                                 q, model->n_curriculas)];
}

bool model_same_teacher(const model *model, int c1, int c2) {
    return model->courses_same_teacher[INDEX2(c1, model->n_courses,
                                              c2, model->n_courses)];
}