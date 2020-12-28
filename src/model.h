#ifndef MODEL_H
#define MODEL_H

// <course> := <CourseID> <Teacher> <# Lectures> <MinWorkingDays> <# Students>
// e.g.     :=   c0001       t000         6            4               130
typedef struct course {
    char *id;
    char *teacher;
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
    char **courses;
} curricula;

// <unavailability_constraint> := <CourseID> <Day> <Day_Period>
// e.g.                        :=    c0001     4       0
typedef struct unavailability_constraint {
    char *course;
    int day;
    int day_period;
} unavailability_constraint;

typedef struct model {
    char *name;
    int n_courses;
    int n_rooms;
    int n_days;
    int n_periods_per_day;
    int n_curriculas;
    int n_unavailability_constraints;
    course *courses;
    room *rooms;
    curricula *curriculas;
    unavailability_constraint *unavailability_constraints;
} model;

void model_init(model *model);

void course_destroy(const course *c);
void room_destroy(const room *r);
void curricula_destroy(const curricula *q);
void unavailability_constraint_destroy(const unavailability_constraint *uc);
void model_destroy(const model *model);

void course_dump(const course *c, char *dump, int size, const char *indent);
void room_dump(const room *r, char *dump, int size, const char *indent);
void curricula_dump(const curricula *q, char *dump, int size, const char *indent);
void unavailability_constraint_dump(const unavailability_constraint *uc,
                                    char *dump, int size, const char *indent);
void model_dump(const model *model, char *dump, int size, const char *indent);


#endif // MODEL_H