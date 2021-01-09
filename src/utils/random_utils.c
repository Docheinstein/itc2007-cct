#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "random_utils.h"
#include <string.h>

void rand_init(uint seed) {
    srandom(seed);
}

int rand_int() {
    return (int) random();
}

double rand_uniform(double lb, double ub) {
    int r = rand_int();
    return lb + ((double) r / RAND_MAX) * (ub - lb);
}

double rand_normal(double mean, double std) {
    // Box-Muller transform, Polar Form
    // u,v € [-1, 1]
    // s = u^2 + v^2 // until s € [0, 1]
    // z0 = u * sqrt(-2 * ln(s) / s)
    // z1 = v * sqrt(-2 * ln(s) / s)
    // G = mean + z0 * std      // mean + z1 *std
    static bool z1_usable = false;
    static double z1;

    if (z1_usable) {
        z1_usable = false;
        return mean + z1 * std;
    }

    double s;
    double u, v;
    do {
        u = rand_uniform(-1, 1);
        v = rand_uniform(-1, 1);
        s = u * u + v * v;
    } while (s <= 0 || s >= 1);

    double r = sqrt(-2 * log(s) / s);
    double z0 = u * r;
    z1 = v * r;

    z1_usable = true;

    return mean + z0 * std;
}

void shuffle(void *array, size_t n, size_t size) {
    if (n <= 1)
        return;

    char tmp[size];
    char *arr = array;
    size_t stride = size * sizeof(char);

    for (size_t i = 0; i < n - 1; ++i) {
        size_t rnd = rand_int();
        size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

        memcpy(tmp, arr + j * stride, size);
        memcpy(arr + j * stride, arr + i * stride, size);
        memcpy(arr + i * stride, tmp, size);
    }
}
