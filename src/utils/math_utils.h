#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif // MAX

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif // MIN

#define RANGIFY(lb, x, ub) (MAX((lb), MIN((ub), (x))))

#endif // MATH_UTILS_H