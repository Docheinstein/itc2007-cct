#ifndef DEF_UTILS_H
#define DEF_UTILS_H

#define INT_TO_PTR(i) ((void *) (long) (i))
#define PTR_TO_INT(p) ((int) (long) (p))

#define LONG_TO_PTR(i) ((void *)(i))
#define PTR_TO_LONG(p) ((long) (p))

#define BOOL_TO_STR(var) ((var) ? "true" : "false")

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif // MAX

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif // MIN

#define RANGIFY(lb, x, ub) (MAX((lb), MIN((ub), (x))))

#endif // DEF_UTILS_H