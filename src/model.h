#ifndef MODEL_H
#define MODEL_H

typedef struct model {
    char *name;
    int n_courses;
    int n_rooms;
    int n_days;
    int n_periods_per_day;
    int n_curricula;
    int n_constraints;
} model;

void model_init(model *model);

#endif // MODEL_H