#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
int debugf(const char *fmt, ...);
#define debug(format, ...) debugf(format "\n", ##__VA_ARGS__)
#else
#define debugf(format, ...)
#define debug(format, ...)
#endif

#endif // DEBUG_H