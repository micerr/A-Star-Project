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
#include "./utility/Item.h"

#define maxWT INT_MAX
#define MAXC 10

extern int errno;
char err[64];

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

typedef struct{   //contains all parameters a thread needs
  pthread_t tid;
  short int id;
  Graph G;
} pthread_par;

/*
  Global variables:
*/
char *finput;
pthread_mutex_t *meLoadV, *meLoadE;
int posV=0, posE=0, n=0, nTh;
struct timeval begin, end;

/*
    Prototypes of static functions
*/
static Edge EDGEcreate(int v, int w, int wt);
static ptr_node NEW(int v, int wt, ptr_node next);
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
static Edge EDGEcreate(int v, int w, int wt) {
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
static ptr_node NEW(int v, int wt, ptr_node next) {
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
    pthread_mutex_lock(meLoadV);
      if(n==0){
        gettimeofday(&begin, 0);
      }
      n++;
    pthread_mutex_unlock(meLoadV);
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
        printf("%d: curr=%d coords=%d %d\n", pars->id, curr, v.coord1, v.coord2);
      #endif
      STinsert(G->coords, v.coord1, v.coord2, curr);
    }
  }else{    
    while(1){
      // get the position
      pthread_mutex_lock(meLoadE);
      curr = posE;
      #ifdef DEBUG
        printf("%d: curr E=%d\n", pars->id, curr);
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
        printf("%d %d %d\n", e.vert1, e.vert2, e.wt);
      #endif
      if (e.vert1 >= 0 && e.vert2 >=0)
        GRAPHinsertE(G, e.vert1-1, e.vert2-1, e.wt); // nodes into file starts from 1
    }
  }
  
  #ifdef TIME
    pthread_mutex_lock(meLoadV);
      nTh--;
      if(nTh==0){
        gettimeofday(&end, 0);
        long int seconds = end.tv_sec - begin.tv_sec;
        long int microseconds = end.tv_usec - begin.tv_usec;
        double elapsed = seconds + microseconds*1e-6;

        printf("Time for reading parallel: %.3f seconds.\n", elapsed);
      }
    pthread_mutex_unlock(meLoadV);
    
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
   | 4 byte | 4 byte | 2 byte | => v1, v2, w
   ... x num edges

  Threads > 1, will load the graph in parallel fashion.

  Parameters: path of the bunary file containing the graph,
  number of threads to be launched.

  Return value: the newly loaded graph.   
*/

Graph GRAPHSequentialLoad(char *fin) {
  int V, nr;
  Graph G;
  Edge e;
  Vert v;

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
    gettimeofday(&begin, 0);
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
      printf("%d: %d %d\n", i, v.coord1, v.coord2);
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
      printf("%d %d %d\n", e.vert1, e.vert2, e.wt);
    #endif

    //insert the edge.
    //By calling GRAPHinsertE a new node in the adjacency list of vert1
    //will be inserted.

    if (e.vert1 >= 0 && e.vert2 >=0)
      GRAPHinsertE(G, e.vert1-1, e.vert2-1, e.wt); // nodes into file starts from 1
  }
  #ifdef TIME
    gettimeofday(&end, 0);
    long int seconds = end.tv_sec - begin.tv_sec;
    long int microseconds = end.tv_usec - begin.tv_usec;
    double elapsed = seconds + microseconds*1e-6;

    printf("Time for reading sequential: %.3f seconds.\n", elapsed);
  #endif

  close(fd);

  printf("\nLoaded graph with %d nodes and %d edges\n", G->V, G->E);
  return G;
}

//  ##### PARALLEL LOAD #####
Graph GRAPHParallelLoad(char *fin, int numThreads){
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
  nTh = numThreads;
  n=0; posE=0; posV=0;

  pthread_par *parameters = malloc(numThreads*sizeof(pthread_par));
  meLoadV = malloc(sizeof(pthread_mutex_t));
  meLoadE = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(meLoadV, NULL);
  pthread_mutex_init(meLoadE, NULL);

  for(i=0; i<numThreads; i++){
    #ifdef DEBUG
      printf("Starting thread: %d\n", i);
    #endif
    parameters[i].id = i;
    parameters[i].G = G;
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

  printf("\nLoaded graph with %d nodes and %d edges\n", G->V, G->E);

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
    ???
*/
void GRAPHedges(Graph G, Edge *a) {
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
void GRAPHinsertE(Graph G, int id1, int id2, int wt) {
  insertE(G, EDGEcreate(id1, id2, wt));
}

/*
  This function encloses all information about an edge (by calling
  EDGEcreate) and remove it forme the graph by calling removeE.

  Parameters: graph from wich to remove the edge, source and destination
  vertices of the edge (the weight of the edge is not relevant).
*/
void GRAPHremoveE(Graph G, int id1, int id2) {
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
  int v = e.vert1, w = e.vert2, wt = e.wt;

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
  This function is in charge of creating the tree containing all 
  minimum-weight paths from one specified source node to all reachable nodes.
  This goal is achieved using the Dijkstra algorithm.

  Parameters: grpah and start node.
*/
void GRAPHspD(Graph G, int id) {
  int v;
  ptr_node t;
  PQ pq = PQinit(G->V);
  int *prio = (int*) malloc(sizeof(int));
  int *neighbour_priority = (int*) malloc(sizeof(int));
  int *path;
  int *mindist;

  path = malloc(G->V * sizeof(int));
  if(path == NULL){
    exit(1);
  }

  mindist = malloc(G->V * sizeof(int));
  if(mindist == NULL){
    exit(1);
  }

  //insert all nodes in the priority queue with total weight equal to infinity
  for (v = 0; v < G->V; v++){
    path[v] = -1;
    mindist[v] = maxWT;
    PQinsert(pq, v, maxWT);
    #if DEBUG
      printf("Inserted node %d priority %d\n", v, maxWT);
    #endif
  }
#if DEBUG
  printf("\n\n\n");
  for(int i=0; i<G->V; i++){
    int ind = PQsearch(pq, i, prio);
    printf("i=%d  indexPQ=%d  prio=%d\n", i, ind, *prio);
  }
  printf("\n\n\n");
#endif

  path[id] = id;
  PQchange(pq, id, 0);
  while (!PQempty(pq)){
    // for(int j=0; j<G->V; j++){
    //   int ind = PQsearch(pq, j, prio);
    //   printf("Node index: %d, Item list index: %d, Priority: %d\n", j, ind, *prio);
    // }

    //At some point it extracts the wrong one (node 2 w priority 2 instead of node 0 w priority 1)
    //Tested with start node=3
    Item min_item = PQextractMin(pq);
    //printf("Extracted %d nodes\n", ++extracted);
  #if DEBUG
    printf("Min extracted is (index): %d\n", min_item.index);
  #endif

    if(min_item.priority != maxWT){
      #if DEBUG
        printf("Found item with min edge\n");
      #endif
      mindist[min_item.index] = min_item.priority;

      for(t=G->ladj[min_item.index]; t!=G->z; t=t->next){
        if(PQsearch(pq, t->v, neighbour_priority) < 0){
          #if DEBUG
           printf("Node: %d already is closed", t->v);
          #endif
          continue;
        }
        #if DEBUG
          printf("Node: %d, Neighbour: %d, New priority: %d, Neighbour priority: %d\n", min_item.index, t->v, min_item.priority + t->wt, *neighbour_priority);
        #endif

        if(min_item.priority + t->wt < (*neighbour_priority)){
          #if DEBUG
            printf("Node %d entered\n", t->v);
          #endif
          PQchange(pq, t->v, min_item.priority + t->wt);
          //printf("New node in path, index: %d", v);
          path[t->v] = min_item.index;
        }
        #if DEBUG
          PQdisplayHeap(pq);
        #endif
      }
    }
  }

  // printf("\n Shortest path tree\n");
  // for (v = 0; v < G->V; v++)
  //   printf("parent of %s is %s \n", STsearchByIndex(G->tab, v), STsearchByIndex(G->tab, st[v]));

  printf("Dijkstra Path:\n");
  for(v = 0; v < G->V; v++){
    printf("Parent %d = %d\n", v, path[v]);
  }

  printf("\n Minimum distances from node %d\n", id);
  for (v = 0; v < G->V; v++)
    printf("mindist[%d] = %d \n", v, mindist[v]);

  PQfree(pq);
}



// Data structures:
// openSet -> Priority queue containing an heap made of Items (Item has index of node and priority (fScore))
// closedSet -> BitArray: 1 if node expanded, 0 otherwise
// path -> the path of the graph (built step by step)
// fScores are embedded within an Item 
// gScores are obtained starting from the fScores and subtracting the value of the heuristic

//openSet is enlarged gradually (inside PQinsert)

// void GRAPHSequentialAStar(Graph G, int start){
//   int v;
//   ptr_node t;
//   PQ openSet = PQinit(1);
//   int *path;

//   path = (int*) malloc(sizeof(int));

  


//   if (path == NULL){
//     return;
//   }

//   path = -1;
//   fScore = maxWT;
//   //PQinsert(pq, fScore, v);
  

//   // for (v = 0; v < G->V; v++){
//   //   path[v] = -1;
//   //   fScore[v] = maxWT;
//   //   PQinsert(pq, fScore, v);
//   // }

//   fScore[start] = 0;
//   path[start] = start;


// }
