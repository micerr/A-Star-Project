#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ST.h"

struct symboltable {Coord *coord; int maxN; int N;};

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
  return st;
}

void STfree(ST st) {
  int i;
  if (st==NULL)
    return;
  free(st->coord);
  free(st);
}

int STsize(ST st) {
  return st->N;
}

void STinsert(ST st, short coord1, short coord2, int i) {
  if (i >= st->maxN) {
    st->coord = realloc(st->coord, (2*st->maxN)*sizeof(Coord));
    if (st->coord == NULL)
      return;
    st->maxN = 2*st->maxN;
  }

  Coord c;
  c = malloc(sizeof(*c));
  c->c1 = coord1; c->c2 = coord2;
  st->coord[i] = c;
  st->N++;
}

int STsearch(ST st, short coord1, short coord2) {
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

