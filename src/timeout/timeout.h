#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <bits/types/sig_atomic_t.h>

extern volatile sig_atomic_t timeout;

void set_timeout(unsigned int seconds);
unsigned int get_timeout_seconds();

#endif // TIMEOUT_H