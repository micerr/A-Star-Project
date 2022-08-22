#ifndef ASTAR_H
#define ASTAR_H

#include "Graph.h"
// dijkstra implementations
int GRAPHspD(Graph G, int id, int end);
int GRAPHcheckAdmissibility(Graph G, int source, int target);

// A-star implementations
void ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord));
void GRAPHSequentialAStar(Graph G, int start, int end, int (*h)(Coord, Coord));

#endif