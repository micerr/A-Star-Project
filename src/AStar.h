#ifndef ASTAR_H
#define ASTAR_H

#define maxWT INT_MAX

#include "Graph.h"


typedef struct thArg_s {
  pthread_t tid;
  Graph G;
  int start, end, numTH;
  int id;
  pthread_mutex_t *meNodes, *meBest, *meOpen;
  pthread_cond_t *cv;
  int (*h)(Coord, Coord);
} thArg_t;




// A-star implementations
void ASTARSequentialAStar(Graph G, int start, int end, int (*h)(Coord, Coord));
void ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord));
void ASTARSimpleParallelV2(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord));
void ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord));
int  GRAPHspD(Graph G, int id, int end); 

#endif