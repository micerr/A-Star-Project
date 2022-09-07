#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>

#include "AStar.h"
#include "PQ.h"
#include "Heuristic.h"
#include "Test.h"
#include "./utility/Item.h"
#include "./utility/Timer.h"



// Data structures:
// openSet -> Priority queue containing an heap made of Items (Item has index of node and priority (fScore))
// closedSet -> Array of int, each cell is the fScore of a node.
// path -> the path of the graph (built step by step)
// fScores are embedded within an Item 
// gScores are obtained starting from the fScores and subtracting the value of the heuristic

//openSet is enlarged gradually (inside PQinsert)

static PQ openSet_PQ;
static int *closedSet, bCost;
static int *hScores;
static int *path, n;
#if defined TIME || defined ANALYTICS
  static Timer timer;
  static int nExtractions;
  static pthread_mutex_t meNExtractions = PTHREAD_MUTEX_INITIALIZER;
#endif 

static void* thFunction(void *par);

Analytics ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type){

  setbuf(stdout, NULL);

  if(G == NULL){
    printf("No graph inserted.\n");
    return NULL;
  }
  int prio;
  Coord dest_coord, coord;
  thArg_t *thArgArray;
  pthread_mutex_t *meNodes, *meBest;
  pthread_cond_t *cv;

  #if defined TIME || defined ANALYTICS
    timer = TIMERinit(numTH);
    nExtractions = 0;
  #endif

  //init the open set (priority queue)
  openSet_PQ = PQinit(G->V, search_type);
  if(openSet_PQ == NULL){
    perror("Error trying to create openSet_PQ: ");
    exit(1);
  }

  //init the closed set (int array)
  closedSet = malloc(G->V * sizeof(int));
  if(closedSet == NULL){
    perror("Error trying to create closedSet: ");
    exit(1);
  }

  //allocate the path array
  path = (int *) malloc(G->V * sizeof(int));
  if(path == NULL){
    perror("Error trying to allocate path array: ");
    exit(1);
  }

  //init hScores array
  hScores = (int *) malloc(G->V * sizeof(int));
  if(hScores == NULL){
    perror("Error trying to allocate hScores: ");
    exit(1);
  }

  for(int i=0; i<G->V; i++){
    closedSet[i]=-1;
    path[i]=-1;
  }
  
  //retrieve coordinates of the target vertex
  dest_coord = STsearchByIndex(G->coords, end);

  //compute all hScores for the desired end node
  for(int v=0; v<G->V; v++){
    coord = STsearchByIndex(G->coords, v);
    hScores[v] = h(coord, dest_coord);    
  }

  //compute starting node's fScore. (gScore = 0)
  prio = hScores[start];

  //insert the starting node in the open set (priority queue)
  PQinsert(openSet_PQ, start, prio);
  path[start] = start;

  //init locks
  meNodes = (pthread_mutex_t *)malloc(sizeof(*meNodes));
  pthread_mutex_init(meNodes, NULL);

  cv = (pthread_cond_t*) malloc(sizeof(*cv));
  pthread_cond_init(cv, NULL);

  n=0;

  meBest = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(meBest, NULL);

  bCost = maxWT;

  //create and launch all threads
  thArgArray = (thArg_t *)malloc(numTH * sizeof(thArg_t));
  if(thArgArray == NULL){
    perror("Error trying to allocate the array of threads' arguments: ");
    exit(1);
  }

  for(int i=0; i<numTH; i++){
    thArgArray[i].id = i;
    thArgArray[i].G = G;
    thArgArray[i].start = start;
    thArgArray[i].end = end;
    thArgArray[i].numTH = numTH;
    thArgArray[i].meNodes = meNodes;
    thArgArray[i].meOpen = NULL;
    thArgArray[i].cv = cv;
    thArgArray[i].meBest = meBest;
    #ifdef DEBUG
      printf("Creo thread %d\n", i);
    #endif
    pthread_create(&thArgArray[i].tid, NULL, thFunction, (void *)&thArgArray[i]);
  }

  for(int i=0; i<numTH; i++)
    pthread_join(thArgArray[i].tid, NULL);

  Analytics stats = NULL;
  #ifdef ANALYTICS
    int size = 0;
    size += PQgetByteSize(openSet_PQ); // openSet
    size += G->V * sizeof(int);     // path
    size += G->V * sizeof(int);     // closedSet
    size += G->V * sizeof(int);     // hscores
    size += sizeof(void *) * 4;     // pointers
    size += sizeof(pthread_mutex_t) * 2 + sizeof(pthread_cond_t);
    size += numTH * sizeof(thArg_t);
    stats = ANALYTICSsave(G, start, end, path, bCost, nExtractions,0,0,0, TIMERgetElapsed(timer), size);
  #endif
  #ifdef TIME
    int expandedNodes = 0;
    for(int i=0;i<G->V;i++){
      if(path[i]>=0)
        expandedNodes++;
    }
    printf("Expanded nodes: %d\n",expandedNodes);
    printf("Total extraction from OpenSet: %d\n", nExtractions);
  #endif

  //printf path and its cost
  #ifndef ANALYTICS
  if(path[end]==-1)
    printf("No path from %d to %d has been found.\n", start, end);
  else{
    printf("Path from %d to %d has been found with cost %d\n", start, end, bCost);
    int hops=0;
    for(int i=end; i!=start; i=path[i]){
      printf("%d <- ", i);
      hops++;
    }
    printf("%d\nHops: %d\n", start, hops);
  }
  #endif

  #if defined TIME || defined ANALYTICS
    TIMERfree(timer);
  #endif

  free(path);
  free(closedSet);
  PQfree(openSet_PQ);
  pthread_mutex_destroy(meNodes);
  pthread_mutex_destroy(meBest);
  pthread_cond_destroy(cv);
  free(cv);
  free(meBest);
  free(meNodes);
  free(thArgArray);
  free(hScores);

  return stats;
}

static void* thFunction(void *par){
  thArg_t *arg = (thArg_t *)par;

  int newGscore, newFscore, Hscore;
  int neighboor_fScore, neighboor_hScore;
  Item extrNode;
  Graph G = arg->G;
  ptr_node t;

  #if defined TIME || defined ANALYTICS
    TIMERstart(timer);
  #endif  
  //until the open set is not empty
  while(1){

    // Termiante Detection
    pthread_mutex_lock(arg->meNodes);
    
    while(PQempty(openSet_PQ)){
      n++;
      #ifdef DEBUG
        printf("nThEmpty: %d\n", n);
      #endif
      if(n == arg->numTH){
        #ifdef DEBUG
          printf("%d:No more nodes to expand, i'll kill everybody\n",arg->id);
        #endif
        pthread_cond_broadcast(arg->cv);
        pthread_mutex_unlock(arg->meNodes);
        #ifdef TIME 
          TIMERstopEprint(timer);
        #endif
        #ifdef ANALYTICS 
          TIMERstop(timer);
        #endif
        pthread_exit(NULL);
      }
      #ifdef DEBUG
        printf("%d: waiting for other nodes\n", arg->id);
      #endif      
      pthread_cond_wait(arg->cv, arg->meNodes);
      #ifdef DEBUG
        printf("%d: woke up\n", arg->id);
      #endif 
      if(n == arg->numTH){
        pthread_mutex_unlock(arg->meNodes);
        #ifdef TIME 
          TIMERstopEprint(timer);
        #endif
        #ifdef ANALYTICS 
          TIMERstop(timer);
        #endif
        #ifdef DEBUG
          printf("%d:Killed\n", arg->id);
        #endif
        pthread_exit(NULL);
      }
    }

    //extract the vertex with the lowest fScore
    // Extract
    extrNode = PQextractMin(openSet_PQ);
    //add the extracted node to the closed set
    #ifdef DEBUG
      printf("%d: closed node:%d\n", arg->id, extrNode.index);
    #endif
    closedSet[extrNode.index] = extrNode.priority; // Closed set is already protected by meExtr
    
    pthread_mutex_unlock(arg->meNodes);

    #ifdef DEBUG
      printf("%d: extracted node:%d prio:%d\n", arg->id, extrNode.index, extrNode.priority);
    #endif

    pthread_mutex_lock(arg->meBest);
    if(extrNode.priority >= bCost){
      pthread_mutex_unlock(arg->meBest);
      #ifdef DEBUG
        printf("%d: f(n)=%d >= f(bestPath)=%d, skip it!\n", arg->id, extrNode.priority, bCost);
      #endif
      continue;
    }
    pthread_mutex_unlock(arg->meBest);

    //retrieve its coordinates
    //extr_coord = STsearchByIndex(G->coords, extrNode.index);
    Hscore = hScores[extrNode.index];
    
    //if the extracted node is the goal one, check if it has been reached with
    // a lower cost, and if necessary update the cost
    if(extrNode.index == arg->end){
      #ifdef DEBUG
        printf("%d: found the target with cost: %d\n", arg->id, extrNode.priority);
      #endif
      pthread_mutex_lock(arg->meBest);
        if(extrNode.priority < bCost){          
          // save the cost
          bCost = extrNode.priority;
        }
      pthread_mutex_unlock(arg->meBest);
      continue;
    }

    #if defined TIME || defined ANALYTICS
      pthread_mutex_lock(&meNExtractions);
        nExtractions++;
      pthread_mutex_unlock(&meNExtractions);
    #endif

    //consider all adjacent vertex of the extracted node
    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      #ifdef DEBUG
        printf("Thread %d is analyzing edge(%d,%d)\n", arg->id, extrNode.index, t->v);
      #endif
      //retrieve coordinates of the adjacent vertex
      // neighboor_coord = STsearchByIndex(G->coords, t->v);
      neighboor_hScore = hScores[t->v]; 

      //cost to reach the extracted node is equal to fScore less the heuristic.
      //newGscore is the sum between the cost to reach extrNode and the
      //weight of the edge to reach its neighboor.
      newGscore = (extrNode.priority - Hscore) + t->wt;
      newFscore = newGscore + neighboor_hScore;

      pthread_mutex_lock(arg->meNodes);

      //check if the extracted node is still closed
      if(closedSet[extrNode.index] < 0){
        pthread_mutex_unlock(arg->meNodes);
        #ifdef DEBUG
          printf("%d: someone re-opened the closed node %d\n", arg->id, extrNode.index);
        #endif
        break;
      }

      // pruning
      if(path[extrNode.index] == t->v){
        pthread_mutex_unlock(arg->meNodes);
        #ifdef DEBUG
          printf("%d: thes successor of %d come from him, avoiding loops\n", arg->id, extrNode.index);
        #endif
        continue;
      }

      //check if adjacent node has been already closed
      if( (neighboor_fScore = closedSet[t->v]) >= 0){
        // n' belongs to CLOSED SET

        #ifdef DEBUG
          printf("%d: found a closed node %d f(n')=%d\n",arg->id, t->v, neighboor_fScore);
        #endif

        //if a lower gScore is found, reopen the vertex
        if(newGscore < neighboor_fScore - neighboor_hScore){

          //remove it from closed set
          closedSet[t->v] = -1;
          #ifdef DEBUG
            printf("%d: a new better path is found for a closed node %d old g(n')=%d, new g(n')= %d, f(n')=%d\n", arg->id, t->v, neighboor_fScore - neighboor_hScore, newGscore, newFscore);
          #endif
          
          //add it to the open set
          PQinsert(openSet_PQ, t->v, newFscore);
          
          pthread_cond_signal(arg->cv);
          if(n>0) n--; // decrease only if there is someone who is waiting

        }else{
          pthread_mutex_unlock(arg->meNodes);
          #ifdef DEBUG
            printf("%d: g1=%d >= g(n')=%d, skip it!\n",arg->id, newGscore, neighboor_fScore - neighboor_hScore);
          #endif
          continue;
        }
      }else{
        if(PQsearch(openSet_PQ, t->v, &neighboor_fScore) < 0){
          //if it doesn't belong to the open set yet, add it
          #ifdef DEBUG
            printf("%d: found a new node %d f(n')=%d\n",arg->id, t->v, newFscore);
          #endif

          PQinsert(openSet_PQ, t->v, newFscore);

          pthread_cond_signal(arg->cv);
          if(n>0) n--; // decrease only if there is someone who is waiting

        }
        //if it belongs to the open set but with a lower gScore, continue
        else if(newGscore >= neighboor_fScore - neighboor_hScore){
          pthread_mutex_unlock(arg->meNodes);
          #ifdef DEBUG
            printf("%d: g1=%d >= g(n')=%d, skip it!\n",arg->id, newGscore, neighboor_fScore - neighboor_hScore);
          #endif
          continue;
        }
        else{
          // change the fScore if this new path is better
          #ifdef DEBUG
            printf("%d: change the f(n')=%d because old g(n')=%d > new g(n')=%d\n", arg->id,newFscore, neighboor_fScore - neighboor_hScore, newGscore );
          #endif

          PQchange(openSet_PQ, t->v, newFscore);

        }
      }

      // change parent
      path[t->v] = extrNode.index;

      pthread_mutex_unlock(arg->meNodes);

    }
  }

  printf("PANIC!!!!!!\n");
  exit(1);
}