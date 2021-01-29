#include "timeout.h"
#include <signal.h>
#include <unistd.h>
#include <log/debug.h>
#include <log/verbose.h>

volatile sig_atomic_t timeout;

static unsigned int timeout_seconds;

void timeout_handler(int sig) {
    timeout = 1;
}

void set_timeout(unsigned int seconds) {
    verbose("Timeout: %u seconds", seconds);
    timeout_seconds = seconds;
    signal(SIGALRM, timeout_handler);
    alarm(seconds);
}

unsigned int get_timeout_seconds() {
    return timeout_seconds;
}


