#include "math_utils.h"

double map(double lb1, double ub1, double lb2, double ub2, double x) {
    if (ub1 == lb1)
        return ub1;
    return lb2 + (ub2 - lb2) * (x - lb1) / (ub1 - lb1);
}
