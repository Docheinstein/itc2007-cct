#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define debugf(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define debug(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

#endif // DEBUG_H