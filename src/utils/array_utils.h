#ifndef ARRAY_UTILS_H
#define ARRAY_UTILS_H

#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))

/* Direct array indexing macros. */

#define INDEX1(i1, n1) ((i1))

#define INDEX2(i2, n2, i1, n1) \
    ((i2) * (n1) + (i1))

#define INDEX3(i3, n3, i2, n2, i1, n1) \
    ((i3) * (n2) * (n1) + (i2) * (n1) + (i1))

#define INDEX4(i4, n4, i3, n3, i2, n2, i1, n1) \
    ((i4) * (n3) * (n2) * (n1) + (i3) * (n2) * (n1) + (i2) * (n1) + (i1))

#define INDEX5(i5, n5, i4, n4, i3, n3, i2, n2, i1, n1) \
    ((i5) * (n4) * (n3) * (n2) * (n1) + (i4) * (n3) * (n2) * (n1) + (i3) * (n2) * (n1) + (i2) * (n1) + (i1))

/* Reverse array indexing macros. */

#define RINDEX1_0(i, n1) ((i))

#define RINDEX2_0(i, n2, n1) \
    ((i) / (n1))
#define RINDEX2_1(i, n2, n1) \
    ((i) % (n1))

#define RINDEX3_0(i, n3, n2, n1) \
    ((i) / ((n2) * (n1)))
#define RINDEX3_1(i, n3, n2, n1) \
    (((i) % ((n2) * (n1))) / (n1))
#define RINDEX3_2(i, n3, n2, n1) \
    (((i) % ((n2) * (n1))) % (n1))

#define RINDEX4_0(i, n4, n3, n2, n1) \
    ((i) / ((n3) * (n2) * (n1)))
#define RINDEX4_1(i, n4, n3, n2, n1) \
    (((i) % ((n3) * (n2) * (n1))) / ((n2) * (n1)))
#define RINDEX4_2(i, n4, n3, n2, n1) \
    (((i) % ((n3) * (n2) * (n1))) % ((n2) * (n1)) / (n1))
#define RINDEX4_3(i, n4, n3, n2, n1) \
    ((((i) % ((n3) * (n2) * (n1))) % ((n2) * (n1))) % (n1))

#endif // ARRAY_UTILS_H