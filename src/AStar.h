#ifndef ASTAR_H
#define ASTAR_H

#include "Graph.h"
// dijkstra implementations
void  GRAPHspD(Graph G, int id, int end);
// A-star implementations
void GRAPHSequentialAStar(Graph G, int start, int end);

#endif