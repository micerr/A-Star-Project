#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

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

typedef struct{
  pthread_t tid;
  short int id;
  Graph G;
} pthread_par;

static Edge EDGEcreate(int v, int w, int wt);
static ptr_node NEW(int v, int wt, ptr_node next);
static void insertE(Graph G, Edge e);
static void removeE(Graph G, Edge e);
static int readCoords(int fd, short int *coord1, short int *coord2, int node);
static int readEdge(int fd, int *n1, int *n2, short int *wt);

static void *loadThread(void *);

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

static int readCoords(int fd, short int *coord1, short int *coord2, int node){
  if(read(fd, coord1, sizeof(short int)) != sizeof(short int)
  || read(fd, coord2, sizeof(short int)) != sizeof(short int)){
    return 1;
  }
  #ifdef DEBUG
    printf("%d: %hd %hd\n", node, *coord1, *coord2);
  #endif
  return 0;
}

static int readEdge(int fd, int *n1, int *n2, short int *wt){
  if(read(fd, n1, sizeof(int)) != sizeof(int)
    || read(fd, n2, sizeof(int)) != sizeof(int)
    || read(fd, wt, sizeof(short int)) != sizeof(short int)){
    return 1;
  }
  #ifdef DEBUG
    printf("%d %d %hd\n", *n1, *n2, *wt);
  #endif
  return 0;
}

/*
  Global variables:
*/
char *finput;
pthread_mutex_t *meLoadV, *meLoadE;
int posV=0, posE=0, edges;

static void *loadThread(void *vpars){
  int ret = 0, fd, curr, id1, id2;
  short int coord1, coord2, wt;
  pthread_par *pars = (pthread_par *) vpars;
  Graph G = pars->G;
  
  if((fd = open(finput, O_RDONLY)) == -1){
    sprintf(err, "opening file thread %d", pars->id);
    perror(err);
    ret = 1;
    pthread_exit(&ret);
  }

  if(pars->id % 2 == 0){
    // reader of nodes coordinates
    while(posV < G->V){
      // get the position
      pthread_mutex_lock(meLoadV);
      curr = posV;
      #ifdef DEBUG
        printf("%d: curr V=%d\n", pars->id, curr);
      #endif
      posV++;
      pthread_mutex_unlock(meLoadV);
      // move the pointer into file
      if(lseek(fd, 2*sizeof(int)+curr*2*sizeof(short int), SEEK_SET) == -1){
        perror("nodes lseek");
        close(fd);
        ret = 1;
        pthread_exit(&ret);
      }
      // read coordinates
      if(readCoords(fd, &coord1, &coord2, curr)){
        sprintf(err, "reading coords of node %d", curr);
        perror(err);
        close(fd);
        ret=1;
        pthread_exit(&ret);
      }
      STinsert(G->coords, coord1, coord2, curr);
    }
  }else{    
    while(posE < edges){
      // get the position
      pthread_mutex_lock(meLoadE);
      curr = posE;
      #ifdef DEBUG
        printf("%d: curr E=%d\n", pars->id, curr);
      #endif
      posE++;
      pthread_mutex_unlock(meLoadE);
      // move the pointer into file
      if(lseek(fd, 2*sizeof(int) + G->V*2*sizeof(short int) + curr*(2*sizeof(int)+sizeof(short int)), SEEK_SET) == -1 ){
        perror("edges lseek");
        close(fd);
        ret = 1;
        pthread_exit(&ret);
      }
      // read edges
      if(readEdge(fd, &id1, &id2, &wt)){
        perror("reading edges of the graph");
        close(fd);
        ret=1;
        pthread_exit(&ret);
      }
      if (id1 >= 0 && id2 >=0)
        GRAPHinsertE(G, id1, id2, wt);
    }
  }

  close(fd);
  pthread_exit(&ret);
}

/*
  Load the graph from a binary file
    
  The standard is the following:
   | 4 byte | 4 byte |  => n. nodes, n. edges
   | 2 byte | 2 byte | => cord1, cord2 node i-esimo
   ... x num nodes
   | 4 byte | 4 byte | 2 byte | => v1, v2, w
   ... x num edges

  Threads > 1, will load the graph in parallel fashion.
   
*/
Graph GRAPHload(char *fin, int numThreads) {
  int V, E, id1, id2, i;
  short int coord1, coord2, wt;
  Graph G;

  int fd = open(fin, O_RDONLY);
  if(fd < 0){
    perror("open graph bin file");
    return NULL;
  }

  if(read(fd, &V, sizeof(int)) != sizeof(int)
    || read(fd, &E, sizeof(int)) != sizeof(int)){
    perror("read num nodes or num edges");
    close(fd);
    return NULL;
  }
  #ifdef DEBUG
    printf("#Nodes: %d, #Edges: %d\n", V, E);
  #endif

  G = GRAPHinit(V);
  if (G == NULL){
    close(fd);
    return NULL;
  }

  if(numThreads > 1){
    close(fd);
    finput = fin;
    edges = E;

    pthread_par *paramiters = malloc(numThreads*sizeof(pthread_par));
    meLoadV = malloc(sizeof(pthread_mutex_t));
    meLoadE = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(meLoadV, NULL);
    pthread_mutex_init(meLoadE, NULL);

    for(i=0; i<numThreads; i++){
      #ifdef DEBUG
        printf("Starting thread: %d\n", i);
      #endif
      paramiters[i].id = i;
      paramiters[i].G = G;
      pthread_create(&(paramiters[i].tid), NULL, loadThread, (void *)&(paramiters[i]));
    }

    int ret = 0;
    int *pRet = &ret;
    int err = 0;
    for(i=0; i<numThreads; i++){
      pthread_join(paramiters[i].tid,(void **) &pRet);
      if(*pRet == 1)
        err = 1;
    }

    pthread_mutex_destroy(meLoadE);
    pthread_mutex_destroy(meLoadV);
    free(meLoadE);
    free(meLoadV);
    free(paramiters);

    if(err){
      GRAPHfree(G);
      return NULL;
    }
    return G;
  }

  // TODO parallelize everything 

  // Reading coordinates of each node
  #ifdef DEBUG
    printf("Nodes coordinates:\n");
  #endif
  for(int i=0; i<V; i++) {
    if(readCoords(fd, &coord1, &coord2, i)){
        sprintf(err, "reading coords of node %d", i);
        perror(err);
        GRAPHfree(G);
        close(fd);
        return NULL;
      }
    STinsert(G->coords, coord1, coord2, i);
  }


  // Reading edges of the graph
  #ifdef DEBUG
    printf("Edges:\n");
  #endif
  for(int i=0; i<E; i++){
    if(readEdge(fd, &id1, &id2, &wt)){
      perror("reading edges of the graph");
      GRAPHfree(G);
      close(fd);
      return NULL;
    }
    if (id1 >= 0 && id2 >=0)
      GRAPHinsertE(G, id1, id2, wt);
  }

  close(fd);

  if(G->E != E){
    printf("Something goes wrong during loading\n");
    return NULL;
  }

  printf("\nLoaded graph with %d nodes and %d edges\n", G->V, G->E);
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
  #ifdef DEBUG
    printf("Edges:\n");
    for(int i=0; i<G->E; i++){
      printf("%d %d %hd\n", a[i].v, a[i].w, a[i].wt);
    }
  #endif

  // TODO parallelize the process

  // writing num nodes and num edges
  if(write(fd, &G->V, sizeof(int)) != sizeof(int)
    || write(fd, &G->E, sizeof(int)) != sizeof(int)){
    perror("writing num nodes and num edges into file");
    close(fd);
    return;
  }
  // writing nodes coordinates
  for(int i=0; i<G->V; i++){
    coords = STsearchByIndex(G->coords, i);
    if(write(fd, &coords->c1, sizeof(short int)) != sizeof(short int)
      || write(fd, &coords->c2, sizeof(short int)) != sizeof(short int)){
      sprintf(err, "writing coords of node %d", i);
      perror(err);
      close(fd);
      return;
    }
  }
  // writing edges
  for(int i=0; i<G->E; i++){
    if(write(fd, &a[i].v, sizeof(int)) != sizeof(int)
      || write(fd, &a[i].w, sizeof(int)) != sizeof(int)
      || write(fd, &a[i].wt, sizeof(short int)) != sizeof(short int)){
      perror("writing edges into file");
      close(fd);
      return;
    }
  }

  printf("Stored graph in bin file: %s", fin);

  close(fd);
}

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
