#ifndef ARRAY_UTILS_H
#define ARRAY_UTILS_H

#define INDEX1(i1, n1) ((i1))

#define INDEX2(i2, n2, i1, n1) \
    ((i2) * (n1) + (i1))

#define INDEX3(i3, n3, i2, n2, i1, n1) \
    ((i3) * (n2) * (n1) + (i2) * (n1) + (i1))

#define INDEX4(i4, n4, i3, n3, i2, n2, i1, n1) \
    ((i4) * (n3) * (n2) * (n1) + (i3) * (n2) * (n1) + (i2) * (n1) + (i1))


#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))

#endif // ARRAY_UTILS_H