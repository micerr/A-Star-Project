#ifndef PQ_H_DEFINED
#define PQ_H_DEFINED

#include "./utility/Item.h"

typedef struct pqueue *PQ;
// typedef struct item Item;


PQ      PQinit(int maxN);
void    PQfree(PQ pq);
int     PQempty(PQ pq);
int     PQmaxSize(PQ pq);
void    PQinsert(PQ pq, int index, float priority);
Item    PQextractMin(PQ pq);
Item    PQgetMin(PQ pq);
void    PQchange (PQ pq, int node_index, float priority);
int     PQsearch(PQ pq, int node_index, float *priority);
void    PQdisplayHeap(PQ pq);

#endif
