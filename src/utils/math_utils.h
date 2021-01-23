#ifndef MATH_UTILS_H
#define MATH_UTILS_H

//#ifndef MAX
//#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
//#endif
//
//#ifndef MIN
//#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
//#endif

#define RANGIFY(lb, x, ub) (MAX((lb), MIN((ub), (x))))

double map(double lb1, double ub1, double lb2, double ub2, double x);

#endif // MATH_UTILS_H