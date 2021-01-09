#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <sys/types.h>

void rand_init(uint seed);

int rand_int();

double rand_uniform(double lb, double ub);
double rand_normal(double mean, double std);

void shuffle(void *array, size_t n, size_t size);


#endif // RANDOM_UTILS_H