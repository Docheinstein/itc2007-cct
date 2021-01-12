#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

#include "solution.h"

typedef struct neighbourhood_iter {
    const solution *solution;
    int *assignments;
    int n_assignments;
    int assignment_index;
    int roomdayslot_index;
} neighbourhood_iter;

void neighbourhood_iter_init(neighbourhood_iter *iter, const solution *sol);
void neighbourhood_iter_destroy(neighbourhood_iter *iter);
bool neighbourhood_iter_next(neighbourhood_iter *iter, int *c1,
                             int *r1, int *d1, int *s1,
                             int *r2, int *d2, int *s2);

bool neighbourhood_swap_assignment(const solution *sol_in, solution *sol_out,
                                   int c1, int r1, int d1, int s1, int r2, int d2, int s2);

#endif // NEIGHBOURHOOD_H