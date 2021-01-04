#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>
#include <stdbool.h>
#include <glib.h>

// <course> := <CourseID> <Teacher> <# Lectures> <MinWorkingDays> <# Students>
// e.g.     :=   c0001       t000         6            4               130
typedef struct course {
    char *id;
    char *teacher_id;
    int n_lectures;
    int min_working_days;
    int n_students;
} course;

// <room> := <RoomID> <Capacity>
// e.g.   :=    B         200
typedef struct room {
    char *id;
    int capacity;
} room;

// <curricula> := <CurriculumID> <# Courses> <CourseID> ... <CourseID>
// e.g.        :=      q000         4         c0001 c0002 c0004 c0005 
typedef struct curricula {
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
} unavailability_constraint;

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

    // Redundant data (for faster access)
    bool * course_belongs_to_curricula;     // b_cq
    char ** teachers;                       // T
    int n_teachers;
    bool * course_taught_by_teacher;        // e_ct
    bool * course_availabilities;           // e_cds

} model;

void model_init(model *model);

void course_set(course *c,
                char *id, char *teacher_id, int n_lectures,
                int min_working_days, int n_students);
void room_set(room *r, char *id, int capacity);
void curricula_set(curricula* q, char *id, int n_courses, char **courses_ids);
void unavailability_constraint_set(unavailability_constraint *uc,
                                   char *course_id, int day, int day_period);

void course_destroy(const course *c);
void room_destroy(const room *r);
void curricula_destroy(const curricula *q);
void unavailability_constraint_destroy(const unavailability_constraint *uc);
void model_destroy(const model *model);

void course_to_string(const course *c, char *buffer, size_t buflen);
void room_to_string(const room *r, char *buffer, size_t buflen);
void curricula_to_string(const curricula *q, char *buffer, size_t buflen);
void unavailability_constraint_to_string(const unavailability_constraint *uc,
                                         char *buffer, size_t buflen);

char * model_to_string(const model *model);

void model_finalize(model *model);

course *model_course_by_id(const model *model, char *id);
room *model_room_by_id(const model *model, char *id);
curricula *model_curricula_by_id(const model *model, char *id);

int model_course_belongs_to_curricula(const model *model, int course_idx, int curricula_idx);
int model_course_taught_by_teacher(const model *model, int course_idx, int teacher_idx);
int model_course_is_available_on_period(const model *model, int course_idx, int day, int slot);

#endif // MODEL_H