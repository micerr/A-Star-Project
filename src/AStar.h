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
void ASTARSequentialAStar(Graph G, int start, int end, int (*h)(Coord, Coord), int search_type);
void ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
void ASTARSimpleParallelV2(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
void ASTARhdaMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
void ASTARhdaNoMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
int  GRAPHspD(Graph G, int id, int end, int search_type); 

#endif