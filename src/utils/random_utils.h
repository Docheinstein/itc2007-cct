#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <sys/types.h>

void rand_set_seed(uint seed);
uint rand_get_seed();

int rand_int();
int rand_int_range(int start, int end);

double rand01();

double rand_uniform(double a, double b);
double rand_normal(double mean, double std);
double rand_triangular(double a, double c, double b);

void shuffle(void *array, size_t n, size_t size);


#endif // RANDOM_UTILS_H