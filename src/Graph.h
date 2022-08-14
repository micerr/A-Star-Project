#ifndef GRAPH_H
#define GRAPH_H

#include "ST.h"

typedef struct edge {
    int v;
    int w;
    int wt;
} Edge;

typedef struct graph *Graph;

Graph GRAPHinit(int V);
void  GRAPHfree(Graph G);
Graph GRAPHload(char *fin, int numThreads);
void  GRAPHinsertE(Graph G, int id1, int id2, int wt);
void  GRAPHremoveE(Graph G, int id1, int id2);
void  GRAPHedges(Graph G, Edge *a);
void  GRAPHspD(Graph G, int id);

#endif
