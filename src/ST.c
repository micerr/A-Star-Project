#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ST.h"

struct symboltable {
  Coord *coord;
  int maxN;
  int N;
  pthread_mutex_t *me;
};

ST STinit(int maxN) {
  ST st;
  st = malloc(sizeof (*st));
  if (st == NULL) {
    printf("Memory allocation error\n");
    return NULL;
  }
  st->coord = malloc(maxN*sizeof(Coord));
  if (st->coord == NULL) {
    printf("Memory allocation error\n");
    free(st);
    return NULL;
  }
  st->maxN = maxN;
  st->N = 0;

  st->me = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(st->me, NULL);
  return st;
}

void STfree(ST st) {
  if (st==NULL)
    return;
  
  for(int i=0; i<st->N; i++)
    free(st->coord[i]);
  free(st->coord);

  pthread_mutex_destroy(st->me);
  free(st->me);

  free(st);
}

int STsize(ST st) {
  int n;
  pthread_mutex_lock(st->me);
    n = st->N;
  pthread_mutex_unlock(st->me);
  return n;
}

int STmaxSize(ST st) {
  int n;
  pthread_mutex_lock(st->me);
    n = st->maxN;
  pthread_mutex_unlock(st->me);
  return n;
}

void  STinsert(ST st, float coord1, float coord2, int i){
  pthread_mutex_lock(st->me);
  if (i >= st->maxN) {
    st->coord = realloc(st->coord, (2*st->maxN)*sizeof(Coord));
    if (st->coord == NULL)
      return;
    st->maxN = 2*st->maxN;
  }

  Coord c;
  c = malloc(sizeof(*c));
  c->c1 = coord1; 
  c->c2 = coord2;
  st->coord[i] = c;
  st->N++;
  pthread_mutex_unlock(st->me);
}

int STsearch(ST st, int coord1, int coord2) {
  int i;
  for(i = 0; i  < st->N; i++)
    if( st->coord[i] != NULL 
      && st->coord[i]->c1 == coord1
      && st->coord[i]->c2 == coord2)
      return i;
  return -1;
}

Coord STsearchByIndex(ST st, int i){
  if (i < 0 || i >= st->N)
    return NULL;
  return (st->coord[i]);
}

