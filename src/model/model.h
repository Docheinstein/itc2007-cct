#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>
#include <stdbool.h>
#include <glib.h>

/*
 * Model of the ITC2007 instances.
 */

// Macro for avoid to define the model constants each time
#define MODEL(m) \
    const int C = (m)->n_courses; \
    const int R = (m)->n_rooms;   \
    const int D = (m)->n_days;   \
    const int S = (m)->n_slots;   \
    const int T = (m)->n_teachers; \
    const int Q = (m)->n_curriculas; \
    const int L = (m)->n_lectures; \
    const model *model = (m)

// Macros for iterate the model's structure easily
#define FOR_C for (int c = 0; c < C; c++)
#define FOR_R for (int r = 0; r < R; r++)
#define FOR_D for (int d = 0; d < D; d++)
#define FOR_S for (int s = 0; s < S; s++)
#define FOR_Q for (int q = 0; q < Q; q++)
#define FOR_T for (int t = 0; t < T; t++)
#define FOR_L for (int l = 0; l < L; l++)

// (implicitly defined by courses)
typedef struct teacher {
    int index;
    char *id;
} teacher;

// <course> := <CourseID> <Teacher> <# Lectures> <MinWorkingDays> <# Students>
// e.g.     :=   c0001       t000         6            4               130
typedef struct course {
    int index;
    char *id;
    char *teacher_id;
    int n_lectures;
    int min_working_days;
    int n_students;

    const teacher *teacher; // redundant, for faster access
} course;

// <room> := <RoomID> <Capacity>
// e.g.   :=    B         200
typedef struct room {
    int index;
    char *id;
    int capacity;
} room;

// <curricula> := <CurriculumID> <# Courses> <CourseID_0> ... <CourseID_N>
// e.g.        :=      q000         4         c0001 c0002 c0004 c0005
typedef struct curricula {
    int index;
    char *id;
    int n_courses;
    char **courses_ids;
} curricula;

// <unavailability_constraint> := <CourseID> <Day> <Day_Period>
// e.g.                        :=    c0001     4       0
typedef struct unavailability_constraint {
    char *course_id;
    int day;
    int slot;

    const course *course; // redundant, for faster access
} unavailability_constraint;

// (implicitly defined by courses)
typedef struct lecture {
    int index;
    const course *course;
} lecture;

/*
 * Model.
 */
typedef struct model {
    char *name;

    int n_courses;
    int n_rooms;
    int n_days;
    int n_slots;
    int n_curriculas;
    int n_unavailability_constraints;

    course *courses;
    room *rooms;
    curricula *curriculas;
    unavailability_constraint *unavailability_constraints;

    // Redundant/Implicit data (for faster access)
    int n_lectures;
    lecture *lectures;

    int n_teachers;
    teacher *teachers;

    GHashTable *course_by_id;
    GHashTable *room_by_id;
    GHashTable *curricula_by_id;
    GHashTable *teacher_by_id;

    GArray **curriculas_of_course;      // [c]
    GArray **courses_of_teacher;        // [t]
    int **courses_of_curricula;         // [q]

    bool *course_belongs_to_curricula;  // [q,c]
    bool *course_taught_by_teacher;     // [c,t]
    bool *course_availabilities;        // [c,d,s]
    bool *courses_share_curricula;      // [c,c,q]
    bool *courses_same_teacher;         // [c,c]

    const char *_filename;
    int _id;
} model;

void model_init(model *model);
void model_destroy(const model *model);

/* Compute the redundant data: must be called by the parser */
void model_finalize(model *model);

course *model_course_by_id(const model *model, char *id);
room *model_room_by_id(const model *model, char *id);
curricula *model_curricula_by_id(const model *model, char *id);
teacher *model_teacher_by_id(const model *model, char *id);

bool model_course_belongs_to_curricula(const model *model, int c, int q);
bool model_course_is_taught_by_teacher(const model *model, int c, int t);
bool model_course_is_available_on_period(const model *model, int c, int d, int s);
int *model_curriculas_of_course(const model *model, int c, int *n_curriculas);
int *model_courses_of_curricula(const model *model, int q, int *n_courses);
int *model_courses_of_teacher(const model *model, int t, int *n_courses);
bool model_share_curricula(const model *model, int c1, int c2, int q);
bool model_same_teacher(const model *model, int c1, int c2);

#endif // MODEL_H