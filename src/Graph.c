#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "Graph.h"
#include "PQ.h"
#include "AStar.h"
#include "Heuristic.h"
#include "./utility/Item.h"
#include "./utility/Timer.h"

#define MAXC 10

extern int errno;
char err[64];

typedef struct{   //contains all parameters a thread needs
  pthread_t tid;
  short int id;
  int startFrom;
  Graph G;
} pthread_par;

/*
  Global variables:
*/
char *finput;
pthread_mutex_t *meLoadV, *meLoadE;
int posV=0, posE=0;
Timer timerLoad;

/*
    Prototypes of static functions
*/
static Edge EDGEcreate(int v, int w, float wt);
static ptr_node NEW(int v, float wt, ptr_node next);
static void insertE(Graph G, Edge e);
static void removeE(Graph G, Edge e);
static void *loadThread(void *);

/*
  This function encloses all elements describing an edge 
  (passed as parameters) in the struct representing the 
  Edge.

  Parameters: index representing source and destination
  vertices and the weight of the edge.
  
  Return value: the structure representing the edge.
*/
static Edge EDGEcreate(int v, int w,float wt) {
  Edge e;
  e.vert1 = v;
  e.vert2 = w;
  e.wt = wt;
  return e;
}

/*
  This function creates a new node to be inserted within the
  adjacency list of a vertex. The newly created node will be
  inserted at the head of the list and will point, as its
  successor, the node that was the head of the list.

  Parameters: index of the destination node, weight of the
  edge and pointer to the node that was the head and now is the
  successor of the newly created node.

  Return value: pointer to the new node.
*/
static ptr_node NEW(int v, float wt, ptr_node next) {
  //allocate the space for the node and check for success
  ptr_node x = malloc(sizeof *x);
  if (x == NULL)
    return NULL;
  
  x->v = v;         //index of the adjacent vertex
  x->wt = wt;       //weight of the edge
  x->next = next;   //set the successor
  return x;
}

/*
  This function creates the structure representing the graph
  and that contains all the information used to describe it.

  Parameter: number of vertices

  Return value: the structure representing the grapf in case of
  success, NULL in case an errore occurred.
*/
Graph GRAPHinit(int V) {
  int v;

  //allocate the structure for the graph
  Graph G = malloc(sizeof *G);
  if (G == NULL)
    return NULL;

  G->V = V;   //save the number of vertices
  G->E = 0;   //set the number edges

  //create the sentinel node 
  G->z = NEW(-1, 0, NULL);
  if (G->z == NULL)
    return NULL;
  
  //allocate the vector of the adjacency lists
  G->ladj = malloc(G->V*sizeof(ptr_node));
  if (G->ladj == NULL)
    return NULL;

  //set the sentinel node as termination of all adjacency lists
  for (v = 0; v < G->V; v++)
    G->ladj[v] = G->z;
  
  //init the Symbol Table
  G->coords = STinit(V);
  if (G->coords == NULL)
    return NULL;
  
  //create anc initialize the mutex to access all lists
  G->meAdj = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(G->meAdj, NULL);
  return G;
}

/*
  This function frees the space used to store all structures
  used to represent the graph.

  Parameter: graph to be freed
*/
void GRAPHfree(Graph G) {
  if(G == NULL){
    return;
  }
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

  pthread_mutex_destroy(G->meAdj);
  free(G->meAdj);

  free(G);
  G = NULL;
  return;
}

/*
  This is the function executed from each thread in order to
  load a graph from a binary file in a parallel fashion.
*/
static void *loadThread(void *vpars){
  int ret = 0, fd, curr, nr;
  Edge e;
  Vert v;
  pthread_par *pars = (pthread_par *) vpars;
  Graph G = pars->G;
  
  if((fd = open(finput, O_RDONLY)) == -1){
    sprintf(err, "opening file thread %d", pars->id);
    perror(err);
    ret = 1;
    pthread_exit(&ret);
  }
  
  #ifdef TIME
    TIMERstart(timerLoad);
  #endif

  if(pars->id % 2 == 0){
    // reader of nodes coordinates
    while(posV < G->V){
      // get the position
      pthread_mutex_lock(meLoadV);
      curr = posV;
      posV++;
      pthread_mutex_unlock(meLoadV);
      // move the pointer into file
      if(lseek(fd, 1*sizeof(int)+curr*sizeof(Vert), SEEK_SET) == -1){
        perror("nodes lseek");
        close(fd);
        ret = 1;
        pthread_exit(&ret);
      }
      // read coordinates
      if(read(fd, &v, sizeof(Vert)) != sizeof(Vert)){
        sprintf(err, "reading coords of node %d", curr);
        perror(err);
        close(fd);
        ret=1;
        pthread_exit(&ret);
      }
      #ifdef DEBUG
        //printf("%d: curr=%d coords=%d %d\n", pars->id, curr, v.coord1, v.coord2);
      #endif
      STinsert(G->coords, v.coord1, v.coord2, curr);
    }
  }else{    
    while(1){
      // get the position
      pthread_mutex_lock(meLoadE);
      curr = posE;
      #ifdef DEBUG
        //printf("%d: curr E=%d\n", pars->id, curr);
      #endif
      posE++;
      pthread_mutex_unlock(meLoadE);
      // move the pointer into file
      if(lseek(fd, sizeof(int) + G->V*sizeof(Vert) + curr*(sizeof(Edge)), SEEK_SET) == -1 ){
        perror("edges lseek");
        close(fd);
        ret = 1;
        pthread_exit(&ret);
      }
      // read edges
      if((nr = read(fd, &e, sizeof(Edge))) != sizeof(Edge)){
        if(nr == 0){
          #ifdef DEBUG
            perror("EOF");
          #endif
          break;
        }
        // Error
        perror("reading edges");
        close(fd);
        ret=1;
        pthread_exit(&ret);
      }
      #ifdef DEBUG
        //printf("%d %d %d\n", e.vert1, e.vert2, e.wt);
      #endif
      if (e.vert1 >= 0 && e.vert2 >=0)
        GRAPHinsertE(G, e.vert1-(pars->startFrom), e.vert2-(pars->startFrom), e.wt); // nodes into file starts from 1
    }
  }
  
  #ifdef TIME
    TIMERstopEprint(timerLoad);
  #endif 


  close(fd);
  pthread_exit(&ret);
}

/*
  Load the graph from a binary file
    
  The standard is the following:
   | 4 byte |  => n. nodes
   | 4 byte | 4 byte | => cord1, cord2 node i-esimo
   ... x num nodes
   | 4 byte | 4 byte | 4 byte | => v1, v2, w
   ... x num edges

  Threads > 1, will load the graph in parallel fashion.

  Parameters: path of the bunary file containing the graph,
  number of threads to be launched.

  Return value: the newly loaded graph.   
*/

Graph GRAPHSequentialLoad(char *fin, int startFrom) {
  int V, nr;
  Graph G;
  Edge e;
  Vert v;
  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

  //open the binary input file
  int fd = open(fin, O_RDONLY);
  if(fd < 0){
    perror("open graph bin file");
    return NULL;
  }

  //read the number of vertices
  if(read(fd, &V, sizeof(int)) != sizeof(int)){
    perror("read num vertices: ");
    close(fd);
    return NULL;
  }
  #ifdef DEBUG
    printf("#Nodes: %d\n", V);
  #endif

  //init the structure that will contain the graph
  G = GRAPHinit(V);
  if (G == NULL){
    close(fd);
    return NULL;
  }

  //  ##### SEQUENTIAL LOAD #####
  // Reading coordinates of each node
  #ifdef DEBUG
    printf("Nodes coordinates:\n");
  #endif
  #ifdef TIME
    TIMERstart(timer);
  #endif

  //read all vertices and their coordinates 
  for(int i=0; i<V; i++) {
    //read coordinates of the vertex
    if(read(fd, &v, sizeof(Vert)) != sizeof(Vert)){
        sprintf(err, "reading coords of node %d", i);
        perror(err);
        GRAPHfree(G);
        close(fd);
        return NULL;
      }
    #ifdef DEBUG
      //printf("%d: %d %d\n", i, v.coord1, v.coord2);
    #endif
    STinsert(G->coords, v.coord1, v.coord2, i);
  }

  // Reading edges of the graph
  #ifdef DEBUG
    printf("Edges:\n");
  #endif

  //read all edges. Keep reading until EOF
  while(1){
    if((nr = read(fd, &e, sizeof(Edge))) != sizeof(Edge)){
      if(nr == 0) break; // EOF
      // Error
      perror("reading edges");
      GRAPHfree(G);
      close(fd);
      return NULL;
    }
    #ifdef DEBUG
      //printf("%d %d %d\n", e.vert1, e.vert2, e.wt);
    #endif

    //insert the edge.
    //By calling GRAPHinsertE a new node in the adjacency list of vert1
    //will be inserted.

    if (e.vert1 >= 0 && e.vert2 >=0)
      GRAPHinsertE(G, e.vert1-startFrom, e.vert2-startFrom, e.wt); // nodes into file starts from 1
  }
  #ifdef TIME
    TIMERstopEprint(timer);
    GRAPHstats(G);
  #endif

  close(fd);

  #ifdef TIME
    TIMERfree(timer);
  #endif

  printf("\nLoaded graph sequential with %d nodes and %d edges\n", G->V, G->E);
  return G;
}

/*  
  ##### PARALLEL LOAD #####
*/
Graph GRAPHParallelLoad(char *fin, int numThreads, int startFrom){
  // int V, i, nr;
  int V, i;
  Graph G;
  // Edge e;
  // Vert v;

  //open the binary input file
  int fd = open(fin, O_RDONLY);
  if(fd < 0){
    perror("open graph bin file");
    return NULL;
  }

  //read the number of vertices
  if(read(fd, &V, sizeof(int)) != sizeof(int)){
    perror("read num vertices: ");
    close(fd);
    return NULL;
  }
  #ifdef DEBUG
    printf("#Nodes: %d\n", V);
  #endif

  //init the structure that will contain the graph
  G = GRAPHinit(V);
  if (G == NULL){
    close(fd);
    return NULL;
  }

  close(fd);
  finput = fin;
  posE=0; posV=0;

  if(numThreads==1) numThreads=2;

  pthread_par *parameters = malloc(numThreads*sizeof(pthread_par));
  meLoadV = malloc(sizeof(pthread_mutex_t));
  meLoadE = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(meLoadV, NULL);
  pthread_mutex_init(meLoadE, NULL);
  #ifdef TIME
    timerLoad = TIMERinit(numThreads);
  #endif

  for(i=0; i<numThreads; i++){
    #ifdef DEBUG
      printf("Starting thread: %d\n", i);
    #endif
    parameters[i].id = i;
    parameters[i].G = G;
    parameters[i].startFrom = startFrom; 
    pthread_create(&(parameters[i].tid), NULL, loadThread, (void *)&(parameters[i]));
  }

  int ret = 0;
  int *pRet = &ret;
  int err = 0;
  for(i=0; i<numThreads; i++){
    pthread_join(parameters[i].tid,(void **) &pRet);
    if(*pRet == 1)
      err = 1;
  }
    
  #ifdef DEBUG
    printf("posE= %d", posE);
  #endif

  printf("\nLoaded graph parallel with %d nodes and %d edges\n", G->V, G->E);

  #ifdef TIME
    TIMERfree(timerLoad);
    GRAPHstats(G);
  #endif  
  pthread_mutex_destroy(meLoadE);
  pthread_mutex_destroy(meLoadV);
  free(meLoadE);
  free(meLoadV);
  free(parameters);

  if(err){
    GRAPHfree(G);
    return NULL;
  }
  return G;
}

/*
  Return into *a an array of the edges
*/
void GRAPHedges(Graph G, Edge *a) {
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  int v, E = 0;
  ptr_node t;
  for (v=0; v < G->V; v++){
    pthread_mutex_lock(G->meAdj);
      for (t=G->ladj[v]; t != G->z; t = t->next)
        a[E++] = EDGEcreate(v, t->v, t->wt);
    pthread_mutex_unlock(G->meAdj);
  }
}

/*
  This function encloses all information about an edge (by calling
  EDGEcreate) and insert it into the graph by calling insertE.

  Parameters: graph in wich to insert the edge, source and destination
  vertices of the edge, weight of the edge.
*/
void GRAPHinsertE(Graph G, int id1, int id2, float wt) {
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  insertE(G, EDGEcreate(id1, id2, wt));
}

/*
  This function encloses all information about an edge (by calling
  EDGEcreate) and remove it forme the graph by calling removeE.

  Parameters: graph from wich to remove the edge, source and destination
  vertices of the edge (the weight of the edge is not relevant).
*/
void GRAPHremoveE(Graph G, int id1, int id2) {
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  removeE(G, EDGEcreate(id1, id2, 0));
}

/*
  This in function is the one that effectivelly insert a new node in the
  adjacency list of the edge's source vertex by calling function NEW.
  The pointer to the node actually at the head is passed to NEW, and
  the pointer to the new node (returned by NEW function) is collected
  such that the head pointer of the list can be properly updated.

  Insertion must be done in mutual exclusion because multiple threads may
  try to add a node to the same adjacency list.

  Parameters: graph and edge to be inserted.
*/
static void  insertE(Graph G, Edge e) {
  int v = e.vert1, w = e.vert2;
  float wt = e.wt;

  pthread_mutex_lock(G->meAdj);
    G->ladj[v] = NEW(w, wt, G->ladj[v]);
    G->E++;
  pthread_mutex_unlock(G->meAdj);
}

/*
  Remove an edge from the graph, so a node from the adjacency list.
  If the node to be removed is pointed by head pointer, operation is
  completed with unit cost; otherwise it searches the correct node to 
  be removed first and then remove it.

  Removal must be done in mutual exclusion because multiple threads may
  try to delete a node from the same adjacency list.

  Parameters: graph and edge to be removed.
*/
static void  removeE(Graph G, Edge e) {
  int v = e.vert1, w = e.vert2;
  ptr_node x;

  pthread_mutex_lock(G->meAdj);
  //check if the node to be removed is the first one
    if (G->ladj[v]->v == w) {
      G->ladj[v] = G->ladj[v]->next;
      G->E--;
    }
    else
    //it's not the first node. Search the correct one and get the pointer
      for (x = G->ladj[v]; x != G->z; x = x->next)
        if (x->next->v == w) {
          x->next = x->next->next;
          G->E--;
    }
  pthread_mutex_unlock(G->meAdj);
}

/*
  Print the dimension of Adjacency list and Coords
*/
void GRAPHstats(Graph G){
  int nCoords, nMaxCoords, sizeCoords, sizeLadj;

  nCoords = STsize(G->coords);
  nMaxCoords = STmaxSize(G->coords);
  sizeCoords = sizeof(struct coord) * nCoords + sizeof(Coord) * nMaxCoords;
  // struct coord = 4B (two unsigned short int), Coord = 8B (it is a pointer)
  printf("nMaxCoords= %d nodes, sizeCoords= %d B (%d MB)\n", nMaxCoords, sizeCoords, sizeCoords>>20 );

  sizeLadj = sizeof(ptr_node) * G->V + sizeof(struct node) * G->E;
  printf("sizeofStructNode= %ld, sizeLadj= %d B (%d MB)\n", sizeof(struct node), sizeLadj, sizeLadj>>20);

}

void GRAPHgetCoordinates(Graph G, int v){
  Coord coord = STsearchByIndex(G->coords, v);
  printf("Coordinates of vertex %d: c1: %f - c2: %f\n", v, coord->c1, coord->c2);
}

void  GRAPHcomputeDistance(Graph G, int v1, int v2){
  Coord coord1, coord2;
  coord1 = STsearchByIndex(G->coords, v1);
  coord2 = STsearchByIndex(G->coords, v2);

  printf("Distance (heuristic): %d\n", Hcoord(coord1, coord2));
}

void  GRAPHgetEdge(Graph G, int start, int end){
  ptr_node t;

  if(start<0 || start>=G->V){
    printf("Invalid starting node\n");
    return;
  }

  if(end<0 || end>=G->V){
    printf("Invalid ending node\n");
    return;
  }

  for(t=G->ladj[start]; t!=G->z; t=t->next)
    if(t->v == end){
      printf("Weight of edge from %d to %d: %f\n", start, end, t->wt);
      return;
    }

  printf("Direct edge from %d to %d does not exist\n", start, end);
  return;
}
