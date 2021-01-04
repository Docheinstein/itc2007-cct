#ifndef DEF_UTILS_H
#define DEF_UTILS_H

#define INT_TO_PTR(i) ((void *) (long) (i))
#define PTR_TO_INT(p) ((int) (long) (p))

#define LONG_TO_PTR(i) ((void *)(i))
#define PTR_TO_LONG(p) ((long) (p))

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif // MIN

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif // MIN

#endif // DEF_UTILS_H