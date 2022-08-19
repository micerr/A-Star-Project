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
#include "./utility/BitArray.h"
#include "./utility/Item.h"
#include "./utility/Timer.h"

#define maxWT INT_MAX

/*
  This function is in charge of creating the tree containing all 
  minimum-weight paths from one specified source node to all reachable nodes.
  This goal is achieved using the Dijkstra algorithm.

  Parameters: graph, start node end node.
*/
int GRAPHspD(Graph G, int id, int end) {
  if(G == NULL){
    printf("No graph inserted.\n");
    return -1;
  }

  int v, hop=0;
  ptr_node t;
  Item min_item;
  PQ pq = PQinit(G->V);
  float neighbour_priority;
  int *path;
  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

  path = malloc(G->V * sizeof(int));
  if(path == NULL){
    exit(1);
  }

  //insert all nodes in the priority queue with total weight equal to infinity
  for (v = 0; v < G->V; v++){
    path[v] = -1;
    PQinsert(pq, v, maxWT);
    #if DEBUG
      printf("Inserted node %d priority %d\n", v, maxWT);
    #endif
  }
  #if DEBUG
    PQdisplayHeap(pq);
  #endif

  #ifdef TIME
    TIMERstart(timer);
  #endif

  path[id] = id;
  PQchange(pq, id, 0);
  while (!PQempty(pq)){
    min_item = PQextractMin(pq);
    #if DEBUG
      printf("Min extracted is (index): %d\n", min_item.index);
    #endif

    if(min_item.index == end){
      // we reached the end node
      break;
    }

    // if the node is discovered
    if(min_item.priority != maxWT){
      // mindist[min_item.index] = min_item.priority;

      for(t=G->ladj[min_item.index]; t!=G->z; t=t->next){
        if(PQsearch(pq, t->v, &neighbour_priority) < 0){
          // The node is already closed
          continue;
        }
        #if DEBUG
          printf("Node: %d, Neighbour: %d, New priority: %.3f, Neighbour priority: %.3f\n", min_item.index, t->v, min_item.priority + t->wt, neighbour_priority);
        #endif

        if(min_item.priority + t->wt < (neighbour_priority)){
          // we have found a better path
          PQchange(pq, t->v, min_item.priority + t->wt);
          path[t->v] = min_item.index;
        }
        #if DEBUG
          PQdisplayHeap(pq);
        #endif
      }
    }
  }

  #ifdef TIME
    TIMERstopEprint(timer);
    int sizeofPath = sizeof(int)*G->V;
    int sizeofPQ = sizeof(PQ*) + PQmaxSize(pq)*sizeof(Item);
    int total = sizeofPath+sizeofPQ;
    printf("sizeofPath= %d B (%d MB), sizeofPQ= %d B (%d MB), TOT= %d B (%d MB)\n", sizeofPath, sizeofPath>>20, sizeofPQ, sizeofPQ>>20, total, total>>20);
  #endif

  // Print the found path
  if(path[v] == -1){
    printf("No path from %d to %d has been found.\n", id, end);
  }else{
    printf("Path from %d to %d has been found with cost %.3f.\n", id, end, min_item.priority);
    for(int v=end; v!=id; ){
      printf("%d <- ", v);
      v = path[v];
      hop++;
    }
    printf("%d\nHops: %d\n",id, hop);
  }
  
  
  #ifdef TIME
    TIMERfree(timer);
  #endif

  free(path);
  PQfree(pq);

  return min_item.priority;
}

/*
  Check the admissibility of a heuristc function for a given graph and (src, dest) couple

  return: 0:= NOT admissible, 1:= admissible
*/
int GRAPHcheckAdmissibility(Graph G,int source, int target){
  int isAmmisible = 1;
  Coord coordTarget = STsearchByIndex(G->coords, target);
  int C = GRAPHspD(G, source, target);
  int h = Hcoord(STsearchByIndex(G->coords, source), coordTarget);
  if( h > C ){
    printf("(%d, %d) is NOT ammissible h(n)= %d, C*(n)= %d\n", source, target, h, C);
    isAmmisible = 0;
  }
  if(isAmmisible){
    printf("is ammissible\n");
  }
  return isAmmisible;
}

// Data structures:
// openSet -> Priority queue containing an heap made of Items (Item has index of node and priority (fScore))
// closedSet -> Array of int, each cell is the fScore of a node.
// path -> the path of the graph (built step by step)
// fScores are embedded within an Item 
// gScores are obtained starting from the fScores and subtracting the value of the heuristic

//openSet is enlarged gradually (inside PQinsert)

PQ openSet_PQ;
float *closedSet;
int *path;
float pathCost = maxWT;
int n;
Timer timer;

typedef struct thArg_s {
  pthread_t tid;
  Graph G;
  int start, end, numTH;
  pthread_mutex_t *meLc, *meLo;
  pthread_cond_t *cv;
} thArg_t;

static void* thFunction(void *par);
#ifdef DEBUG
int nb=0;
pthread_mutex_t mub = PTHREAD_MUTEX_INITIALIZER;
sem_t sem;
#endif 

void ASTARSimpleParallel(Graph G, int start, int end, int numTH){
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  float prio;
  Coord dest_coord, coord;
  thArg_t *thArgArray;
  pthread_mutex_t *meLc, *meLo;
  pthread_cond_t *cv;
  int hop = 0;

  //init the open set (priority queue)
  openSet_PQ = PQinit(1);
  if(openSet_PQ == NULL){
    perror("Error trying to create openSet_PQ: ");
    exit(1);
  }

  //init the closed set (int array)
  closedSet = malloc(G->V * sizeof(float));
  if(closedSet == NULL){
    perror("Error trying to create closedSet: ");
    exit(1);
  }
  for(int i=0; i<G->V; i++){
    closedSet[i]=-1;
  }

  //allocate the path array
  path = (int *) malloc(G->V * sizeof(int));
  if(path == NULL){
    perror("Error trying to allocate path array: ");
    exit(1);
  }

  //retrieve coordinates of the target vertex
  dest_coord = STsearchByIndex(G->coords, end);

  //compute starting node's fScore. (gScore = 0)
  coord = STsearchByIndex(G->coords, start);
  prio = Hcoord(coord, dest_coord);

  //insert the starting node in the open set (priority queue)
  PQinsert(openSet_PQ, start, prio);
  path[start] = start;

  //init locks
  meLc = (pthread_mutex_t *)malloc(sizeof(*meLc));
  pthread_mutex_init(meLc, NULL);

  meLo = (pthread_mutex_t *)malloc(sizeof(*meLo));
  pthread_mutex_init(meLo, NULL);
  
  cv = (pthread_cond_t*) malloc(sizeof(*cv));
  pthread_cond_init(cv, NULL);

  n=0;
  pathCost=maxWT;
  #ifdef DEBUG
    nb=numTH;
    sem_init(&sem, 0, 0);
  #endif

  //create and launch all threads
  thArgArray = (thArg_t *)malloc(numTH * sizeof(thArg_t));
  if(thArgArray == NULL){
    perror("Error trying to allocate the array of threads' arguments: ");
    exit(1);
  }

  #ifdef TIME
    timer = TIMERinit(numTH);
  #endif

  for(int i=0; i<numTH; i++){
    thArgArray[i].G = G;
    thArgArray[i].start = start;
    thArgArray[i].end = end;
    thArgArray[i].numTH = numTH;
    thArgArray[i].meLc = meLc;
    thArgArray[i].meLo = meLo;
    thArgArray[i].cv = cv;
    #ifdef DEBUG
      printf("Creo thread %d\n", i);
    #endif
    pthread_create(&thArgArray[i].tid, NULL, thFunction, (void *)&thArgArray[i]);
  }

  for(int i=0; i<numTH; i++)
    pthread_join(thArgArray[i].tid, NULL);

  //printf path and its cost
  if(closedSet[end] < 0)
    printf("No path from %d to %d has been found.\n", start, end);
  else{
    printf("Path from %d to %d has been found with cost %.3f.\n", start, end, pathCost);
    for(int v=end; v!=start; ){
      printf("%d <- ", v);
      v = path[v];
      hop++;
    }
    printf("%d\nHops: %d\n", start, hop);
  }


  #ifdef TIME
    TIMERfree(timer);
    int sizeofPath = sizeof(int)*G->V;
    int sizeofClosedSet = sizeof(float)*G->V;
    int sizeofPQ = sizeof(PQ*) + PQmaxSize(openSet_PQ)*sizeof(Item);
    int total = sizeofPath+sizeofClosedSet+sizeofPQ;
    printf("sizeofPath= %d B (%d MB *2), sizeofClosedSet= %d B (%d MB), sizeofOpenSet= %d B (%d MB), TOT= %d B (%d MB)\n", sizeofPath, sizeofPath>>20, sizeofClosedSet, sizeofClosedSet>>20, sizeofPQ, sizeofPQ>>20, total, total>>20);
  #endif
  free(path);
  free(closedSet);
  PQfree(openSet_PQ);
  pthread_mutex_destroy(meLo);
  pthread_mutex_destroy(meLc);
  free(meLc);
  free(meLo);
  free(thArgArray);

  return;
}

static void* thFunction(void *par){
  thArg_t *arg = (thArg_t *)par;

  int flag, isNotConsistent = 0;
  float newGscore, newFscore;
  float neighboor_gScore, neighboor_fScore, neighboor_hScore;
  Item extrNode;
  Graph G = arg->G;
  Coord dest_coord, extr_coord, neighboor_coord;
  ptr_node t;

  //retrieve coordinates of the target vertex
  dest_coord = STsearchByIndex(G->coords, arg->end);

  #ifdef DEBUG
  // barrier to start thread at the same time
    pthread_mutex_lock(&mub);
      nb--;
      if(nb==0){
        for(int i=0;i<arg->numTH;i++){
          sem_post(&sem);
        }
      }
    pthread_mutex_unlock(&mub);
    sem_wait(&sem);
  #endif

  #ifdef TIME
    TIMERstart(timer);
  #endif  
  //until the open set is not empty
  while(1){

    // Termiante Detection
    
    pthread_mutex_lock(arg->meLo);
    
    while(PQempty(openSet_PQ)){
      n++;
      #ifdef DEBUG
        printf("nThEmpty: %d\n", n);
      #endif
      if(n == arg->numTH){
        #ifdef DEBUG
          printf("%ld:No more nodes to expand, i'll kill everybody\n",arg->tid);
        #endif
        pthread_cond_broadcast(arg->cv);
        pthread_mutex_unlock(arg->meLo);
        #ifdef TIMER
          TIMERstopEprint(timer);
        #endif
        pthread_exit(NULL);
      }
      #ifdef DEBUG
        printf("%ld: waiting for other nodes\n", arg->tid);
      #endif      
      pthread_cond_wait(arg->cv, arg->meLo);
      #ifdef DEBUG
        printf("%ld: waked up\n", arg->tid);
      #endif 
      if(n == arg->numTH){
        pthread_mutex_unlock(arg->meLo);
        #ifdef DEBUG
          printf("%ld:Killed\n", arg->tid);
        #endif
        pthread_exit(NULL);
      }
    }

    //extract the vertex with the lowest fScore
    extrNode = PQextractMin(openSet_PQ);
    pthread_mutex_unlock(arg->meLo);
    #ifdef DEBUG
      printf("%ld: extracted node:%d prio:%f\n", arg->tid, extrNode.index, extrNode.priority);
    #endif

    if(extrNode.priority >= pathCost){
      #ifdef DEBUG
        printf("%ld: f(n)=%f >= f(bestPath)=%f, skip it!\n", arg->tid, extrNode.priority, pathCost);
      #endif
      continue;
    }
      

    pthread_mutex_lock(arg->meLc);
    //add the extracted node to the closed set
    closedSet[extrNode.index] = extrNode.priority;
    pthread_mutex_unlock(arg->meLc);

    //retrieve its coordinates
    extr_coord = STsearchByIndex(G->coords, extrNode.index);
    
    //if the extracted node is the goal one, check if it has been reached with
    // a lower cost, and if necessary update the cost
    if(extrNode.index == arg->end){
      #ifdef DEBUG
        pthread_mutex_lock(arg->meLo);
        printf("%ld: found the target with cost: %f\n", arg->tid, extrNode.priority);
        for(int v=arg->end; v!=arg->start; ){
          printf("%d <- ", v);
          v = path[v];
        }
        printf("%d\n", arg->start);
        pthread_mutex_unlock(arg->meLo);
      #endif
      pthread_mutex_lock(arg->meLo);
        if(extrNode.priority < pathCost);
        pathCost = extrNode.priority;
      pthread_mutex_unlock(arg->meLo);
      continue;
    }

    //consider all adjacent vertex of the extracted node
    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      //retrieve coordinates of the adjacent vertex
      neighboor_coord = STsearchByIndex(G->coords, t->v);
      neighboor_hScore = Hcoord(neighboor_coord, dest_coord); 

      //cost to reach the extracted node is equal to fScore less the heuristic.
      //newGscore is the sum between the cost to reach extrNode and the
      //weight of the edge to reach its neighboor.
      newGscore = (extrNode.priority - Hcoord(extr_coord, dest_coord)) + t->wt;
      #ifdef DEBUG
        printf("%ld: n=%d n'=%d c(n, n')=%d h(n')=%f g1=%f\n",arg->tid, extrNode.index, t->v, t->wt, neighboor_hScore, newGscore);
      #endif

      //check if adjacent node has been already closed
      pthread_mutex_lock(arg->meLc);
      if(closedSet[t->v] > 0){
        // n' belongs to CLOSED SET
        if(!isNotConsistent){
          printf("The heuristic function is NOT consistent\n");
          isNotConsistent = 1;
        }

        neighboor_fScore = closedSet[t->v];
        neighboor_gScore = neighboor_fScore - neighboor_hScore;
        #ifdef DEBUG
          printf("%ld: found a closed node %d f(n')=%f\n",arg->tid, t->v, neighboor_fScore);
        #endif

        //if a lower gScore is found, reopen the vertex
        if(newGscore < neighboor_gScore){
          //remove it from closed set
          closedSet[t->v] = -1;
          
          //add it to the open set
          newFscore = newGscore + neighboor_hScore;

          pthread_mutex_lock(arg->meLo);
            #ifdef DEBUG
              printf("%ld: a new better path is found for a closed node %d old g(n')=%f, new g(n')= %f, f(n')=%f\n", arg->tid, t->v, neighboor_gScore, newGscore, newFscore);
            #endif
            PQinsert(openSet_PQ, t->v, newFscore);
            #ifdef DEBUG
              printf("%ld: awaking someone\n", arg->tid);
            #endif 
            pthread_cond_signal(arg->cv);
            if(n>0) n--; // decrease only if there is someone who is waiting
            path[t->v] = extrNode.index;

          pthread_mutex_unlock(arg->meLo);
          pthread_mutex_unlock(arg->meLc);
        }
        else{
          pthread_mutex_unlock(arg->meLc);
          continue;
        }
      }
      //if it hasn't been closed yet
      else{
        pthread_mutex_unlock(arg->meLc);
        pthread_mutex_lock(arg->meLo);
        flag = PQsearch(openSet_PQ, t->v, &neighboor_fScore);
        neighboor_gScore = neighboor_fScore - neighboor_hScore;

        //if it doesn't belong to the open set yet, add it
        if(flag<0){
          newFscore = newGscore + neighboor_hScore;
          #ifdef DEBUG
            printf("%ld: found a new node %d f(n')=%f\n",arg->tid, t->v, newFscore);
          #endif
          PQinsert(openSet_PQ, t->v, newFscore);
          #ifdef DEBUG
            printf("%ld: awaking someone\n", arg->tid);
          #endif
          pthread_cond_signal(arg->cv);
          if(n>0) n--;  //decrease only if there is someone who is waiting     
          path[t->v] = extrNode.index;
          pthread_mutex_unlock(arg->meLo);
        }
        //if it belongs to the open set but with a lower gScore, continue
        else if(neighboor_gScore <= newGscore){
          #ifdef DEBUG
            printf("%ld: g1=%f >= g(n')=%f, skip it!\n",arg->tid, newGscore, neighboor_gScore);
          #endif
          pthread_mutex_unlock(arg->meLo);
          continue;
        }
        else{
          newFscore = newGscore + neighboor_hScore;
          #ifdef DEBUG
            printf("%ld: change the f(n')=%f because old g(n')=%f > new g(n')=%f\n", arg->tid,newFscore, neighboor_gScore, newGscore );
          #endif
          PQchange(openSet_PQ, t->v, newFscore);
          path[t->v] = extrNode.index;
          pthread_mutex_unlock(arg->meLo);
        }
      }
    }
  }

  printf("PANIC!!!!!!\n");
  exit(1);
}