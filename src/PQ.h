#ifndef PQ_H_DEFINED
#define PQ_H_DEFINED

#include "./utility/Item.h"

#define LINEAR_SEARCH 1
#define CONSTANT_SEARCH 2
#define PARALLEL_SEARCH 3

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
int     PQgetByteSize(PQ pq);

#endif
