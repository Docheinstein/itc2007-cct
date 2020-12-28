#include "model.h"
#include <stddef.h>
#include <stdio.h>

void model_init(model *model) {
    model->name = NULL;
    model->n_courses = 0;
    model->n_rooms = 0;
    model->n_days = 0;
    model->n_periods_per_day = 0;
    model->n_curricula = 0;
    model->n_constraints = 0;
}

void model_dump(model *model, char *dump, int size) {
    model_dump_indent(model, dump, size, "");
}

void model_dump_indent(model *model, char *dump, int size, const char *indent) {
    snprintf(dump, size,
       "%sname = %s\n"
       "%s# courses = %d\n"
       "%s# rooms = %d\n"
       "%s# days = %d\n"
       "%s# n_periods_per_day = %d\n"
       "%s# n_curricula = %d\n"
       "%s# n_constraints = %d\n"
       ,
       indent, model->name,
       indent, model->n_courses,
       indent, model->n_rooms,
       indent, model->n_days,
       indent, model->n_periods_per_day,
       indent, model->n_curricula,
       indent, model->n_constraints
   );
}
