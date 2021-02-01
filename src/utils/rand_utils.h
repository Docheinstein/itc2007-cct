#ifndef RAND_UTILS_H
#define RAND_UTILS_H

#include <stddef.h>

void rand_set_seed(unsigned int seed);
unsigned int rand_get_seed();

/* Random int between 0 and INT_MAX */
int rand_int();

/* Random int between start (inclusive) and end (exclusive) */
int rand_range(int start, int end);

/* Random double between a and b */
double rand_uniform(double a, double b);

/* Random following a normal distribution
 * with the given mean and standard deviation */
double rand_normal(double mean, double std);

/* Random following a triangular distribution
 * with lower bound=a, upper bound=b and mode=c */
double rand_triangular(double a, double c, double b);

void shuffle(void *array, size_t n, size_t size);

#endif // RAND_UTILS_H