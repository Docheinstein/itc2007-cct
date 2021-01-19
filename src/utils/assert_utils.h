#ifndef ASSERT_UTILS_H
#define ASSERT_UTILS_H

#include <stdio.h>

#if defined(__GNUC__)
#  define LIKELY(expr) (__builtin_expect ((expr), 1))
#  define UNLIKELY(expr) (__builtin_expect ((expr), 0))
#  define UNUSED __attribute__((__unused__))
#else
#  define LIKELY(expr) (expr)
#  define UNLIKELY(expr) (expr)
#  define UNUSED
#endif

#ifdef ASSERT
#define assert(cond) do { \
    if LIKELY(cond); \
    else assertion_message(__FILE__, __LINE__, __func__, #cond); \
} while(0)
#else
#define assert(cond) do { } while(0)
#endif

void assertion_message(const char *file, int line,
                       const char *func, const char *message);

#endif // ASSERT_UTILS_H