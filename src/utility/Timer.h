#ifndef TIMER_H
#define TIMER_H

typedef struct timer *Timer;

Timer TIMERinit(int numThread);
void TIMERfree(Timer timer);
void TIMERstart(Timer timer);
double TIMERstopEprint(Timer timer);

#endif