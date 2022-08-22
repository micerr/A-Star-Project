#ifndef ASTAR_H
#define ASTAR_H

#define maxWT INT_MAX

#include "Graph.h"



// A-star implementations
void ASTARSequentialAStar(Graph G, int start, int end, int (*h)(Coord, Coord));
void ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord));


#endif