#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

typedef struct timer *Timer;

Timer TIMERinit(int numThread);
void TIMERfree(Timer timer);
void TIMERstart(Timer timer);
double TIMERstopEprint(Timer timer);
double TIMERstop(Timer timer);
double TIMERgetElapsed(Timer t);

struct timeval TIMERgetTime(void);
double computeTime(struct timeval begin, struct timeval end);

#endif