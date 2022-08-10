#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "Graph.h"
#include "PQ.h"

#define maxWT INT_MAX
#define MAXC 10

extern int errno;
char err[64];

typedef struct node *ptr_node;
struct node {
  int v;
  int wt;
  ptr_node next;
};

struct graph {
  int V;
  int E;
  ptr_node *ladj;
  ST coords;
  ptr_node z;
};

static Edge  EDGEcreate(int v, int w, int wt);
static ptr_node  NEW(int v, int wt, ptr_node next);
static void  insertE(Graph G, Edge e);
static void  removeE(Graph G, Edge e);

static Edge EDGEcreate(int v, int w, int wt) {
  Edge e;
  e.v = v;
  e.w = w;
  e.wt = wt;
  return e;
}

static ptr_node NEW(int v, int wt, ptr_node next) {
  ptr_node x = malloc(sizeof *x);
  if (x == NULL)
    return NULL;
  x->v = v;
  x->wt = wt;
  x->next = next;
  return x;
}

Graph GRAPHinit(int V) {
  int v;
  Graph G = malloc(sizeof *G);
  if (G == NULL)
    return NULL;

  G->V = V;
  G->E = 0;
  G->z = NEW(-1, 0, NULL);
  if (G->z == NULL)
    return NULL;
  G->ladj = malloc(G->V*sizeof(ptr_node));
  if (G->ladj == NULL)
    return NULL;
  for (v = 0; v < G->V; v++)
    G->ladj[v] = G->z;
  G->coords = STinit(V);
  if (G->coords == NULL)
    return NULL;
  return G;
}

void GRAPHfree(Graph G) {
  int v;
  ptr_node t, next;
  for (v=0; v < G->V; v++)
    for (t=G->ladj[v]; t != G->z; t = next) {
      next = t->next;
      free(t);
    }
  STfree(G->coords);
  free(G->ladj);
  free(G->z);
  free(G);
}

Graph GRAPHload(char *fin) {
  int V, E, id1, id2;
  short int coord1, coord2, wt;
  Graph G;

  /*
   * The standard is the following:
   *  | 4 byte | 4 byte |  => n. nodes, n. edges
   *  | 2 byte | 2 byte | => cord1, cord2 node i-esimo
   *  ... x num nodes
   *  | 4 byte | 4 byte | 2 byte | => v1, v2, w
   *  ... x num edges
   */

  int fd = open(fin, O_RDONLY);
  if(fd < 0){
    perror("open graph bin file");
    return NULL;
  }

  if(read(fd, &V, sizeof(int)) != sizeof(int)
    || read(fd, &E, sizeof(int)) != sizeof(int)){
    perror("read num nodes or num edges");
    return NULL;
  }

  G = GRAPHinit(V);
  if (G == NULL)
    return NULL;
  // TODO parallelize everything 

  // Reading coordinates of each node
  for(int i=0; i<V; i++) {
    if(read(fd, &coord1, sizeof(short int)) != sizeof(short int)
      || read(fd, &coord2, sizeof(short int)) != sizeof(short int)){
        sprintf(err, "reading coords of node %d", i);
        perror(err);
        GRAPHfree(G);
        return NULL;
      }
    STinsert(G->coords, coord1, coord2, i);
  }

  // Reading edges of the graph
  for(int i=0; i<E; i++){
    if(read(fd, &id1, sizeof(int)) != sizeof(int)
      || read(fd, &id2, sizeof(int)) != sizeof(int)
      || read(fd, &wt, sizeof(short int)) != sizeof(short int)){
      perror("reading edges of the graph");
      GRAPHfree(G);
      return NULL;
    }
    if (id1 >= 0 && id2 >=0)
      GRAPHinsertE(G, id1, id2, wt);
  }

  close(fd);
  return G;
}

void  GRAPHedges(Graph G, Edge *a) {
  int v, E = 0;
  ptr_node t;
  for (v=0; v < G->V; v++)
    for (t=G->ladj[v]; t != G->z; t = t->next)
      a[E++] = EDGEcreate(v, t->v, t->wt);
}

void GRAPHstore(Graph G, char *fin) {
  Edge *a;
  Coord coords;

  /*
   * The standard is the following:
   *  | 4 byte | 4 byte |  => n. nodes, n. edges
   *  | 2 byte | 2 byte | => cord1, cord2 node i-esimo
   *  ... x num nodes
   *  | 4 byte | 4 byte | 2 byte | => v1, v2, w
   *  ... x num edges
   */

  int fd = open(fin, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

  a = malloc(G->E * sizeof(Edge));
  if (a == NULL)
    return;
  GRAPHedges(G, a);

  // TODO parallelize the process

  // writing num nodes and num edges
  if(write(fd, &G->V, sizeof(int)) != sizeof(int)
    || write(fd, &G->E, sizeof(int)) != sizeof(int)){
    perror("writing num nodes and num edges into file");
    return;
  }
  // writing nodes coordinates
  for(int i=0; i<G->V; i++){
    coords = STsearchByIndex(G->coords, i);
    if(write(fd, &coords->c1, sizeof(short int)) != sizeof(short int)
      || write(fd, &coords->c2, sizeof(short int)) != sizeof(short int)){
      sprintf(err, "writing coords of node %d", i);
      perror(err);
      return;
    }
  }
  // writing edges
  for(int i=0; i<G->E; i++){
    if(write(fd, &a[i].v, sizeof(int)) != sizeof(int)
      || write(fd, &a[i].w, sizeof(int)) != sizeof(int)
      || write(fd, &a[i].wt, sizeof(short int)) != sizeof(short int)){
      perror("writing edges into file");
      return;
    }
  }

  close(fd);
}

// int GRAPHgetIndex(Graph G, char *label) {
//   int id;
//   id = STsearch(G->tab, label);
//   if (id == -1) {
//     id = STsize(G->tab);
//     STinsert(G->tab, label, id);
//   }
//   return id;
// }

void GRAPHinsertE(Graph G, int id1, int id2, int wt) {
  insertE(G, EDGEcreate(id1, id2, wt));
}

void GRAPHremoveE(Graph G, int id1, int id2) {
  removeE(G, EDGEcreate(id1, id2, 0));
}

static void  insertE(Graph G, Edge e) {
  int v = e.v, w = e.w, wt = e.wt;

  G->ladj[v] = NEW(w, wt, G->ladj[v]);
  G->E++;
}

static void  removeE(Graph G, Edge e) {
  int v = e.v, w = e.w;
  ptr_node x;
  if (G->ladj[v]->v == w) {
    G->ladj[v] = G->ladj[v]->next;
    G->E--;
  }
  else
    for (x = G->ladj[v]; x != G->z; x = x->next)
      if (x->next->v == w) {
        x->next = x->next->next;
        G->E--;
  }
}

void GRAPHspD(Graph G, int id) {
  int v;
  ptr_node t;
  PQ pq = PQinit(G->V);
  int *st, *mindist;
  st = malloc(G->V*sizeof(int));
  mindist = malloc(G->V*sizeof(int));
  if ((st == NULL) || (mindist == NULL))
    return;

  for (v = 0; v < G->V; v++){
    st[v] = -1;
    mindist[v] = maxWT;
    PQinsert(pq, mindist, v);
  }

  mindist[id] = 0;
  st[id] = id;
  PQchange(pq, mindist, id);

  while (!PQempty(pq)) {
    if (mindist[v = PQextractMin(pq, mindist)] != maxWT) {
      for (t=G->ladj[v]; t!=G->z ; t=t->next)
        if (mindist[v] + t->wt < mindist[t->v]) {
          mindist[t->v] = mindist[v] + t->wt;
          PQchange(pq, mindist, t->v);
          st[t->v] = v;
        }
    }
  }

  // printf("\n Shortest path tree\n");
  // for (v = 0; v < G->V; v++)
  //   printf("parent of %s is %s \n", STsearchByIndex(G->tab, v), STsearchByIndex(G->tab, st[v]));

  // printf("\n Minimum distances from node %s\n", STsearchByIndex(G->tab, id));
  // for (v = 0; v < G->V; v++)
  //   printf("mindist[%s] = %d \n", STsearchByIndex(G->tab, v), mindist[v]);

  PQfree(pq);
}
