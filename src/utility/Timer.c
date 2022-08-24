#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "Timer.h"

struct timer{
    struct timeval begin, end;
    pthread_mutex_t *me;
    int n1, n2;
    double elapsed;
};

/*
    Initialize the Timer

    numThread: number of thread, if the timer is used in a parallel code
*/
Timer TIMERinit(int numThread){
    Timer t;
    t = malloc(sizeof(*t));

    t->me = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(t->me, NULL);
    
    t->n1 = 0;
    t->n2 = numThread;
    t->elapsed = 0;
    return t;
}

void TIMERfree(Timer t){
    pthread_mutex_destroy(t->me);
    free(t->me);
    free(t);
    t = NULL;
}

/*
  Start the timer
*/
void TIMERstart(Timer t){
    pthread_mutex_lock(t->me);
      if(t->n1==0){
        gettimeofday(&(t->begin), 0);
      }
      t->n1++;
    pthread_mutex_unlock(t->me);
}

/*
  Stop the timer and print the seconds

  return:
  double elapsed time
*/
double TIMERstopEprint(Timer t){
  t->elapsed = 0.0;
  pthread_mutex_lock(t->me);
    t->n2--;
    if(t->n2==0){
      gettimeofday(&(t->end), 0);

      t->elapsed = computeTime(t->begin, t->end);

      if(t->elapsed < 0.010)
        printf("Time spent: %.6f seconds.\n", t->elapsed);
      else
        printf("Time spent: %.3f seconds.\n", t->elapsed);
      
    }
  pthread_mutex_unlock(t->me);
  return t->elapsed;
}

double TIMERstop(Timer t){
  t->elapsed = 0.0;
  pthread_mutex_lock(t->me);
    t->n2--;
    if(t->n2==0){
      gettimeofday(&(t->end), 0);

      t->elapsed = computeTime(t->begin, t->end);
    }
  pthread_mutex_unlock(t->me);
  return t->elapsed;
}

/*
  call the method of sys/time.h, gettimeofday

  return:
    struct timeval with the current time
*/
struct timeval TIMERgetTime(){
  struct timeval tv;
  gettimeofday(&(tv), 0);
  return tv;
}

/*
    Compute the difference between two struct timeval in seconds
*/
double computeTime(struct timeval begin, struct timeval end){
  long int seconds = end.tv_sec - begin.tv_sec;
  long int microseconds = end.tv_usec - begin.tv_usec;
  return seconds + microseconds*1e-6;
}

/*
  return the time from begin to end or to the current moment if not end.
*/
double TIMERgetElapsed(Timer t){
  if(t->elapsed == 0.0 
    && t->begin.tv_usec == 0){
    // never started
    return -1;
  }
  return t->elapsed;
}
