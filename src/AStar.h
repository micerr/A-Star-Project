#ifndef ASTAR_H
#define ASTAR_H

#define maxWT INT_MAX

#include "Graph.h"
#include "Test.h"
#include "Hash.h"


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
Analytics GRAPHspD(Graph G, int start, int end, int search_type); 
Analytics ASTARSequentialAStar(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
Analytics ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
Analytics ASTARSimpleParallelV2(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
Analytics ASTARhdaMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type, int (*hfunc)(Hash h, int v));
Analytics ASTARhdaNoMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type, int (*hfunc)(Hash h, int v));

#endif