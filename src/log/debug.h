#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

/*
 * Debug statements.
 * Enabled only if compiled with -DDEBUG=1 or -DDEBUG=2.
 * (with cmake: -DCMAKE_C_FLAGS="-DDEBUG=1")
 */

#if DEBUG >= 1
#define DEBUG1
#define debug(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while(0)
#endif

#if DEBUG >= 2
#define DEBUG2
#define debug2(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define debug2(fmt, ...) do {} while(0)
#endif

#endif // DEBUG_H