#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "Timer.h"

struct timer{
    struct timeval begin, end;
    pthread_mutex_t *me;
    int n1, n2;
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
    return t;
}

void TIMERfree(Timer t){
    pthread_mutex_destroy(t->me);
    free(t->me);
    free(t);
    t = NULL;
}

void TIMERstart(Timer t){
    pthread_mutex_lock(t->me);
      if(t->n1==0){
        gettimeofday(&(t->begin), 0);
      }
      t->n1++;
    pthread_mutex_unlock(t->me);
}

double TIMERstopEprint(Timer t){
    double elapsed = 0.0;
    pthread_mutex_lock(t->me);
      t->n2--;
      if(t->n2==0){
        gettimeofday(&(t->end), 0);
        long int seconds = t->end.tv_sec - t->begin.tv_sec;
        long int microseconds = t->end.tv_usec - t->begin.tv_usec;
        elapsed = seconds + microseconds*1e-6;

        printf("Time spent: %.3f seconds.\n", elapsed);
      }
    pthread_mutex_unlock(t->me);
    return elapsed;
}
