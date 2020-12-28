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

// <curricula> := <CurriculumID> <# Courses> <MemberID> ... <MemberID>
// e.g.        :=      q000         4         c0001 c0002 c0004 c0005 
typedef struct curricula {
    char *id;
    int n_courses;
    char **member_ids;
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
    int n_curricula;
    int n_unavailability_constraints;
    course *courses;
    room *rooms;
    curricula *curriculas;
    unavailability_constraint *unavailability_constraints;
} model;

void course_init(course *course);
void course_dump(const course *course, char *dump, int size, const char *indent);

void model_init(model *model);
void model_destroy(model *model);

void model_dump(const model *model, char *dump, int size, const char *indent);

#endif // MODEL_H