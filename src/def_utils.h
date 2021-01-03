#ifndef DEF_UTILS_H
#define DEF_UTILS_H

#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))

#define INT_TO_PTR(i) ((void *) (long) (i))
#define PTR_TO_INT(p) ((int) (long) (p))

#define LONG_TO_PTR(i) ((void *)(i))
#define PTR_TO_LONG(p) ((long) (p))

#endif // DEF_UTILS_H