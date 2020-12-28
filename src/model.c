#include "model.h"
#include <stddef.h>

void model_init(model *model) {
    model->name = NULL;
    model->n_courses = 0;
    model->n_rooms = 0;
    model->n_days = 0;
    model->n_periods_per_day = 0;
    model->n_curricula = 0;
    model->n_constraints = 0;
}