#ifndef PQ_H_DEFINED
#define PQ_H_DEFINED

#include "./utility/Item.h"

typedef struct pqueue *PQ;


PQ      PQinit(int maxN, int type);
void    PQfree(PQ pq);
int     PQempty(PQ pq);
int     PQmaxSize(PQ pq);
void    PQinsert(PQ pq, int index, int priority);
Item    PQgetMin(PQ pq);
Item    PQextractMin(PQ pq);
void    PQchange (PQ pq, int node_index, int priority);
int     PQsearch(PQ pq, int node_index, int *priority);
void    PQdisplayHeap(PQ pq);
float   PQgetPriority(PQ pq, int index);

#endif
