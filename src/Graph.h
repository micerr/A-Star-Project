#ifndef GRAPH_H
#define GRAPH_H

#include "ST.h"
#include "./utility/Item.h"

//struct representing a vertex
typedef struct{
  int coord1;
  int coord2;
} Vert;

//struct representing an edge
typedef struct __attribute__((__packed__)) edge_s{
  int vert1, vert2;
  short int wt;
} Edge;

typedef struct node *ptr_node;
struct node {     //element of the adjacency list
  int v;          //edge's destination vertex
  int wt;         //weight of the edge
  ptr_node next;  //pointer to the next node
};

struct graph { 
  int V;    //number of vertices
  int E;    //number of edges
  ptr_node *ladj;           //array of adjacency lists
  pthread_mutex_t *meAdj;   //mutex to access adjacency lists
  ST coords;    //symbol table to retrieve information about vertices
  ptr_node z;   //sentinel node. Used to indicate the end of a list
};

typedef struct graph *Graph;

Graph GRAPHinit(int V);
void  GRAPHfree(Graph G);
Graph GRAPHSequentialLoad(char *fin);
Graph GRAPHParallelLoad(char *fin, int numThreads);
void  GRAPHinsertE(Graph G, int id1, int id2, int wt);
void  GRAPHremoveE(Graph G, int id1, int id2);
void  GRAPHedges(Graph G, Edge *a);
void  GRAPHspD(Graph G, int id, int end);

int   GRAPHcc(Graph G);

// A-star implementations
void GRAPHSequentialAStar(Graph G, int start, int end);
#endif
