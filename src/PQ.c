#include <stdlib.h>
#include <stdio.h>
#include "PQ.h"

struct pqueue { Item *A; int *qp; int heapsize; };

struct item {
  int index;
  int fScore;
};

static void Swap(PQ pq, int n1, int n2);
static void Heapify(PQ pq, int *priority, int i);
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

Item ITEMinit(int index, int fScore){
  Item *item = (Item*) malloc(sizeof(*item));

  if(item == NULL){
    exit(1);
  }

  item->index = index;
  item->fScore = fScore;

  return item;
}

PQ PQinit(int maxN) {
  int i;
  PQ pq = malloc(sizeof(*pq));
  pq->A = malloc(maxN*sizeof(*item));
  pq->qp = malloc(maxN*sizeof(int));
  for (i=0; i < maxN; i++)
    pq->qp[i] = -1;
  pq->heapsize = 0;
  return pq;
}

void PQfree(PQ pq) {
  free(pq->qp);
  free(pq->A);
  free(pq);
}

int PQempty(PQ pq) {
  return pq->heapsize == 0;
}

// Inside the while the position of two nodes in the heap is exchanged
// This happens because the node we want to insert has a lower priority value
// wrt his parent. In addition to the exchange of the nodes, also the qp
// vector has to be updated (qp contains the index of a node inside the heap)
void PQinsert (PQ pq, int index, int priority){
  Item *item = ITEMinit(index, priority);

  int i;
  i = pq->heapsize++;
  pq->qp[item->index] = i;

  while (i>=1 && ((pq->A[PARENT(i)])->fScore > item->fScore)) {
    pq->A[i] = pq->A[PARENT(i)];
    pq->qp[pq->A[i]] = i;
    i = (i-1)/2;
  }
  pq->A[i] = item;
  pq->qp[item->index] = i;

  free(item);
  return;
}

static void Swap(PQ pq, int n1, int n2){
  int temp;
  int ind1 = -1, ind2 = -1;
  int fScore1, fScore2;

  while(int i=0; i<pq->heapsize; i++){
    if((pq->A[i])->index == n1){
      ind1 = i;
    }

    if((pq->A[i])->index == n2){
      ind2 = i;
    }

    if((ind1 != -1) && (ind2 != -1)){
      break;
    }
  }

  if((ind1 == -1) || (ind2 == -1)){
    printf("Index not found\n");
    exit(1);
  }

  temp = pq->A[ind1];
  pq->A[ind1] = pq->A[ind2];
  pq->A[ind2] = temp;


  
  n1 = pq->A[n1];
  n2 = pq->A[n2];
  temp = pq->qp[n1];
  pq->qp[n1] = pq->qp[n2];
  pq->qp[n2] = temp;

  return;
}

static void Heapify(PQ pq, int *priority, int i) {
  int l, r, smallest;
  l = LEFT(i);
  r = RIGHT(i);
  if (l < pq->heapsize && (priority[pq->A[l]] < priority[pq->A[i]]))
    smallest = l;
  else
    smallest = i;
  if (r < pq->heapsize && (priority[pq->A[r]] < priority[pq->A[smallest]]))
    smallest = r;
  if (smallest != i) {
    Swap(pq, i,smallest);
	Heapify(pq, priority, smallest);
  }
  return;
}

int PQextractMin(PQ pq, int *priority) {
  int k;
  Swap (pq, 0, pq->heapsize-1);
  k = pq->A[pq->heapsize-1];
  pq->heapsize--;
  Heapify(pq, priority, 0);
  return k;
}

void PQchange (PQ pq, int *priority, Item k) {
  int pos = pq->qp[k];
  Item temp = pq->A[pos];

  while (pos>=1 && (priority[pq->A[PARENT(pos)]] > priority[pq->A[pos]])) {
    pq->A[pos] = pq->A[PARENT(pos)];
    pq->qp[pq->A[pos]] = pos;
    pos = (pos-1)/2;
  }
  pq->A[pos] = temp;
  pq->qp[temp] = pos;

  Heapify(pq, priority, pos);
  return;
}

//PQsearch


