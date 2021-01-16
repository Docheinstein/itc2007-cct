#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#if DEBUG >= 2
#define DEBUG2
#define debug2(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define debug2(fmt, ...)
#endif

#endif // DEBUG_H