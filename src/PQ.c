#include <stdlib.h>
#include <stdio.h>
#include "PQ.h"
#include "./utility/Item.h"

struct pqueue { Item *A; int *qp; int heapsize; };


static void Swap(PQ pq, int n1, int n2);
static void Heapify(PQ pq, int i);
static int LEFT(int i);
static int RIGHT(int i);
static int PARENT(int i);

static int LEFT(int i) {
  return i*2+1;
}

static int RIGHT(int i) {
  return i*2+2;
}

static int PARENT(int i) {
  return (i-1)/2;
}


PQ PQinit(int maxN) {
  int i;
  PQ pq = malloc(sizeof(*pq));
  pq->A = malloc(maxN*sizeof(*item));
  pq->heapsize = 0;

  return pq;
}

void PQfree(PQ pq) {
  free(pq->A);
  free(pq);
}

int PQempty(PQ pq) {
  return pq->heapsize == 0;
}


void PQinsert (PQ pq, int node_index, int priority){
  Item *item = ITEMinit(node_index, priority);

  int i;
  i = pq->heapsize++;

  while (i>=1 && ((pq->A[PARENT(i)])->priority > item->priority)) {
    pq->A[i] = pq->A[PARENT(i)];
    i = PARENT(i);
  }

  pq->A[i] = item;

  free(item);
  return;
}

static void Swap(PQ pq, int n1, int n2){
  Item temp;

  temp  = pq->A[n1];
  pq->A[n1] = pq->A[n2];
  pq->A[n2] = temp;

  return;
}

static void Heapify(PQ pq, int i) {
  int l, r, largest;
  l = LEFT(i);
  r = RIGHT(i);
  if ( (l < pq->heapsize) && ( (pq->A[l])->priority < (pq->A[i])->priority) )
    largest = l;
  else
    largest = i;
  if ( (r < pq->heapsize) && ( (pq->A[r])->priority < (pq->A[i])->priority) )
    largest = r;
  if (largest != i) {
    Swap(pq, i,largest);
	Heapify(pq, largest);
  }
}

/*
Searches for a specific node inside the Item array and returns it's index 
and the priority value inside the priority pointer
*/
int PQsearch(PQ pq, int node_index, int *priority){
  for(int i=0; i<pq->heapsize; i++){
    if(node_index == (pq->A[i])->index){
      if(priority == NULL){
        return i;
      }

      *priority = (pq->A[i])->priority;
      return i;
    }
  }
}

Item PQextractMin(PQ pq) {
  Item item;
  Swap (pq, 0,pq->heapsize-1);
  item = pq->A[pq->heapsize-1];
  pq->heapsize--;
  Heapify(pq, 0);

  return item;
}

void PQchange (PQ pq, int node_index, int priority) {
  int item_index = PQsearch(pq, node_index, NULL);
  Item item = pq->A[item_index];

  while( (item_index>=1) && ((pq->A[PARENT(item_index)])->priority > item->priority)) {
    pq->A[item_index] = pq->A[PARENT(item_index)];
	  item_index = PARENT(item_index);
  }

  pq->A[item_index] = item;
  Heapify(pq, item_index);

  return;
}



