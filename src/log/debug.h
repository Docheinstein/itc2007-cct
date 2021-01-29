#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define debug_if(cond, fmt, ...) do { if (cond) printf(fmt "\n", ##__VA_ARGS__); } while(0)
#else
#define debug(fmt, ...) do {} while(0)
#define debug_if(cond, fmt, ...) do {} while(0)
#endif

#if DEBUG >= 2
#define DEBUG2
#define debug2(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define debug2_if(cond, fmt, ...) do { if (cond) printf(fmt "\n", ##__VA_ARGS__); } while(0)
#else
#define debug2(fmt, ...) do {} while(0)
#define debug2_if(cond, fmt, ...) do {} while(0)
#endif

#if DEBUG >= 3
#define DEBUG2
#define debug3(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define debug3_if(cond, fmt, ...) do { if (cond) printf(fmt "\n", ##__VA_ARGS__); } while(0)
#else
#define debug3(fmt, ...) do {} while(0)
#define debug3_if(cond, fmt, ...) do {} while(0)

#endif

#endif // DEBUG_H