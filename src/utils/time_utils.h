#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "time.h"

/* Current milliseconds of clock */
long clk();

/* Current milliseconds of real time */
long ms();

/* Sleeps for ms */
void mssleep(long ms);

#endif // TIME_UTILS_H