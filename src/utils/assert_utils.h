#ifndef ASSERT_UTILS_H
#define ASSERT_UTILS_H

// Just tips for the compiler
#if defined(__GNUC__)
#  define LIKELY(expr) (__builtin_expect ((expr), 1))
#  define UNLIKELY(expr) (__builtin_expect ((expr), 0))
#  define UNUSED __attribute__((__unused__))
#else
#  define LIKELY(expr) (expr)
#  define UNLIKELY(expr) (expr)
#  define UNUSED
#endif

/* Assert (cond) to be true.
 * `assert_real` is always compiled, use with caution */
#define assert_real(cond) do { \
    if LIKELY(cond); \
    else assertion_message(__FILE__, __LINE__, __func__, #cond); \
} while(0)

/* `assert` is compiled only with -DASSERT */
#ifdef ASSERT
#define assert(cond) assert_real(cond)
#else
#define assert(cond) do { } while(0)
#endif

void assertion_message(const char *file, int line,
                       const char *func, const char *message);

#endif // ASSERT_UTILS_H