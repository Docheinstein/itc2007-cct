#include "timeout.h"
#include <signal.h>
#include <unistd.h>
#include "log/verbose.h"

volatile sig_atomic_t timeout = 0;

void timeout_handler(int sig) {
    timeout = 1;
}

void set_timeout(unsigned int seconds) {
    verbose("Timeout: %u seconds", seconds);
    signal(SIGALRM, timeout_handler);
    alarm(seconds);
}

