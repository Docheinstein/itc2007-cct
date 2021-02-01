#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <bits/types/sig_atomic_t.h>

/*
 * Global timeout variable.
 * Can be safely accessed from everywhere and from
 * the signal handler as well.
 * If it is true, the `seconds` given to `set_timeout`
 * have been elapsed.
 */
extern volatile sig_atomic_t timeout;

/* Set an alarm that cause `timeout` to be true after the given number of seconds. */
void set_timeout(unsigned int seconds);

#endif // TIMEOUT_H