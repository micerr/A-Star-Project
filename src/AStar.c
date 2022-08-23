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

char spinner[] = "|/-\\";
int spin = 0, isNotConsistent;


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
  int *path, *mindist;
  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

  path = malloc(G->V * sizeof(int));
  if(path == NULL){
    exit(1);
  }

  mindist = malloc(G->V * sizeof(int));

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
    PQdisplayHeap(pq);
  #endif

  #ifdef TIME
    TIMERstart(timer);
  #endif

  path[id] = id;
  PQchange(pq, id, 0);
  mindist[id] = 0;
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
    if(mindist[min_item.index] != maxWT){
      // mindist[min_item.index] = min_item.priority;

      for(t=G->ladj[min_item.index]; t!=G->z; t=t->next){
        // se è nella PQ
        // PQsearch(pq, t->v, &neighbour_priority);
        if(min_item.priority + t->wt < mindist[t->v]){
          mindist[t->v] = min_item.priority + t->wt;
          PQchange(pq, t->v, min_item.priority + t->wt);
          path[t->v] = min_item.index;
        }
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
    printf("Path from %d to %d has been found with cost %.3d.\n", id, end, min_item.priority);
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

  //free(mindist);
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
  if(C == -1) return -1;
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
void GRAPHSequentialAStar(Graph G, int start, int end, int (*h)(Coord coord1, Coord coord2)){
  if(G == NULL){
    printf("No graph inserted.\n");
    return ;
  }

  int v, hop=0;
  ptr_node t;
  Item extrNode;
  PQ openSet;
  int *path, *closedSet, *hScores;
  int newGscore, newFscore;
  int hScore, neighboor_hScore, neighboor_fScore;
  int flag;
  Coord coord, dest_coord;

  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

  openSet = PQinit(G->V);
  if(openSet == NULL){
    perror("Error trying to create openSet: ");
    exit(1);
  }

  path = malloc(G->V * sizeof(int));
  if(path == NULL){
    perror("Error trying to allocate path: ");
    exit(1);
  }

  closedSet = malloc(G->V * sizeof(int));
  if(closedSet == NULL){
    perror("Error trying to allocate closedSet: ");
    exit(1);
  }

  hScores = malloc(G->V * sizeof(int));
  if(hScores == NULL){
    perror("Error trying to allocate hScores: ");
    exit(1);
  }

  dest_coord = STsearchByIndex(G->coords, end);

  for (v = 0; v < G->V; v++){
    path[v] = -1;
    closedSet[v] = -1;
    coord = STsearchByIndex(G->coords, v);
    hScores[v] = h(coord, dest_coord);
    #if DEBUG
      printf("Inserted node %d priority %d\n", v, maxWT);
    #endif
  }
  #if DEBUG
    PQdisplayHeap(openSet);
  #endif

  #ifdef TIME
    TIMERstart(timer);
  #endif

  path[start] = start;
  closedSet[start] = 0 + hScores[start];

  PQinsert(openSet, start, closedSet[start]);

  while (!PQempty(openSet)){
    //extract node
    extrNode = PQextractMin(openSet);
    //printf("extrNode.index: %d\n", extrNode.index);
    #if DEBUG
      printf("Min extracted is (index): %d\n", extrNode.index);
    #endif

    //add to the closed set
    closedSet[extrNode.index] = extrNode.priority;

    //it it is the destination node, end computation
    if(extrNode.index == end){
      // we reached the end node
      break;
    }

    hScore = hScores[extrNode.index];

    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      neighboor_hScore = hScores[t->v];
      newGscore = (extrNode.priority - hScore) + t->wt;
      newFscore = newGscore + neighboor_hScore;

      //if it belongs to the closed set
      if(closedSet[t->v] > -1){
        if(newGscore < (closedSet[t->v] - neighboor_hScore)){
          closedSet[t->v] = -1;
          // printf("%d is adding %d (f(n)=%d)\n", extrNode.index, t->v, newFscore);
          PQinsert(openSet, t->v, newFscore);
        }
        else
          continue;
      }else{  //it doesn't belong to closed set
        flag = PQsearch(openSet, t->v, &neighboor_fScore);

        //if it doesn't belong to the open set
        if(flag < 0){
          PQinsert(openSet, t->v, newFscore);
          // printf("%d is adding %d (f(n)=%d)\n", extrNode.index, t->v, newFscore);
        }
        
        else if(newGscore >= (neighboor_fScore - neighboor_hScore))
          continue;
        else{
          // printf("%d is changing priority of %d (f(n)=%d)\n", extrNode.index, t->v, newFscore);
          PQchange(openSet, t->v, newFscore);
        }
      }
      path[t->v] = extrNode.index;
    }

  }

  #ifdef TIME
    TIMERstopEprint(timer);
    int sizeofPath = sizeof(int)*G->V;
    int sizeofPQ = sizeof(PQ*) + PQmaxSize(openSet)*sizeof(Item);
    int total = sizeofPath+sizeofPQ;
    printf("sizeofPath= %d B (%d MB), sizeofPQ= %d B (%d MB), TOT= %d B (%d MB)\n", sizeofPath, sizeofPath>>20, sizeofPQ, sizeofPQ>>20, total, total>>20);
  #endif

  // Print the found path
  if(path[v] == -1){
    printf("No path from %d to %d has been found.\n", start, end);
  }else{
    printf("Path from %d to %d has been found with cost %.3d.\n", start, end, extrNode.priority);
    for(int v=end; v!=start; ){
      printf("%d <- ", v);
      v = path[v];
      hop++;
    }
    printf("%d\nHops: %d\n",start, hop);
  }
  
  
  #ifdef TIME
    TIMERfree(timer);
  #endif

  PQfree(openSet);
  free(path);
  free(closedSet);
  free(hScores);
  return;
}

// Data structures:
// openSet -> Priority queue containing an heap made of Items (Item has index of node and priority (fScore))
// closedSet -> Array of int, each cell is the fScore of a node.
// path -> the path of the graph (built step by step)
// fScores are embedded within an Item 
// gScores are obtained starting from the fScores and subtracting the value of the heuristic

//openSet is enlarged gradually (inside PQinsert)

PQ openSet_PQ;
int *closedSet, bCost;
int *hScores;
int *path, n;
Timer timer;

typedef struct thArg_s {
  pthread_t tid;
  Graph G;
  int start, end, numTH;
  int id;
  pthread_mutex_t *meNodes, *meBest, *meOpen;
  pthread_cond_t *cv;
  int (*h)(Coord, Coord);
} thArg_t;

static void* thFunction(void *par);

void ASTARSimpleParallel(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord)){

  setbuf(stdout, NULL);

  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  int prio;
  Coord dest_coord, coord;
  thArg_t *thArgArray;
  pthread_mutex_t *meNodes, *meBest, *meOpen;
  pthread_cond_t *cv;

  #ifdef TIME
    timer = TIMERinit(numTH);
  #endif

  //init the open set (priority queue)
  openSet_PQ = PQinit(G->V);
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
  meNodes = (pthread_mutex_t *)malloc(G->V * sizeof(*meNodes));
  for(int i=0; i<G->V; i++){
    pthread_mutex_init(&(meNodes[i]), NULL);
  }

  meOpen = (pthread_mutex_t *)malloc(sizeof(*meOpen));
  pthread_mutex_init(meOpen, NULL);

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
    thArgArray[i].cv = cv;
    thArgArray[i].meBest = meBest;
    thArgArray[i].meOpen = meOpen;
    thArgArray[i].h = h;
    #ifdef DEBUG
      printf("Creo thread %d\n", i);
    #endif
    pthread_create(&thArgArray[i].tid, NULL, thFunction, (void *)&thArgArray[i]);
  }

  for(int i=0; i<numTH; i++)
    pthread_join(thArgArray[i].tid, NULL);

  #ifdef TIME
    TIMERfree(timer);
    int sizeofPath = sizeof(int)*G->V;
    int sizeofClosedSet = sizeof(float)*G->V;
    int sizeofPQ = sizeof(PQ*) + PQmaxSize(openSet_PQ)*sizeof(Item);
    int total = sizeofPath+sizeofClosedSet+sizeofPQ;
    int expandedNodes = 0;
    printf("sizeofPath= %d B (%d MB *2), sizeofClosedSet= %d B (%d MB), sizeofOpenSet= %d B (%d MB), TOT= %d B (%d MB)\n", sizeofPath, sizeofPath>>20, sizeofClosedSet, sizeofClosedSet>>20, sizeofPQ, sizeofPQ>>20, total, total>>20);
    for(int i=0;i<G->V;i++){
      if(path[i]>=0)
        expandedNodes++;
    }
    printf("Expanded nodes: %d\n",expandedNodes);
  #endif

  //printf path and its cost
  if(path[0]==-1)
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


  free(path);
  free(closedSet);
  PQfree(openSet_PQ);
  for(int i=0; i<G->V; i++){
    pthread_mutex_destroy(&(meNodes[i]));
  }
  pthread_mutex_destroy(meBest);
  pthread_mutex_destroy(meOpen);
  pthread_cond_destroy(cv);
  free(cv);
  free(meBest);
  free(meOpen);
  free(meNodes);
  free(thArgArray);
  free(hScores);

  return;
}

static void* thFunction(void *par){
  thArg_t *arg = (thArg_t *)par;

  int newGscore, newFscore, Hscore;
  int neighboor_fScore, neighboor_hScore;
  Item extrNode;
  Graph G = arg->G;
  ptr_node t;

  isNotConsistent = 0;

  #ifdef TIME
    TIMERstart(timer);
  #endif  
  //until the open set is not empty
  while(1){
    printf("%c\b", spinner[(spin++)%4]);

    // Termiante Detection
    pthread_mutex_lock(arg->meOpen);
    
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
        pthread_mutex_unlock(arg->meOpen);
        #ifdef TIME
          TIMERstopEprint(timer);
        #endif
        pthread_exit(NULL);
      }
      #ifdef DEBUG
        printf("%d: waiting for other nodes\n", arg->id);
      #endif      
      pthread_cond_wait(arg->cv, arg->meOpen);
      #ifdef DEBUG
        printf("%d: woke up\n", arg->id);
      #endif 
      if(n == arg->numTH){
        pthread_mutex_unlock(arg->meOpen);
        #ifdef TIME
          TIMERstopEprint(timer);
        #endif
        #ifdef DEBUG
          printf("%d:Killed\n", arg->id);
        #endif
        pthread_exit(NULL);
      }
    }

    //extract the vertex with the lowest fScore
    extrNode = PQgetMin(openSet_PQ);

    if(pthread_mutex_trylock(&(arg->meNodes[extrNode.index])) != 0){
      pthread_mutex_unlock(arg->meOpen);
      continue;
    }

    extrNode = PQextractMin(openSet_PQ);
    closedSet[extrNode.index] = extrNode.priority;

    pthread_mutex_unlock(&(arg->meNodes[extrNode.index]));

    pthread_mutex_unlock(arg->meOpen);

    //add the extracted node to the closed set

    #ifdef DEBUG
      printf("%d: extracted and closed node:%d prio:%d\n", arg->id, extrNode.index, extrNode.priority);
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

      pthread_mutex_lock(&(arg->meNodes[t->v]));

      //check if adjacent node has been already closed
      if( (neighboor_fScore = closedSet[t->v]) >= 0){
        // n' belongs to CLOSED SET

        #ifdef DEBUG
          printf("%d: found a closed node %d f(n')=%d\n",arg->id, t->v, neighboor_fScore);
        #endif

        //if a lower gScore is found, reopen the vertex
        if(newGscore < neighboor_fScore - neighboor_hScore){

          if(!isNotConsistent){
            printf("The heuristic function is NOT consistent\n");
            isNotConsistent = 1;
          }

          //remove it from closed set
          closedSet[t->v] = -1;
          #ifdef DEBUG
            printf("%d: a new better path is found for a closed node %d old g(n')=%d, new g(n')= %d, f(n')=%d\n", arg->id, t->v, neighboor_fScore - neighboor_hScore, newGscore, newFscore);
          #endif

          pthread_mutex_lock(arg->meOpen);

          //add it to the open set
          PQinsert(openSet_PQ, t->v, newFscore);
          
          pthread_cond_signal(arg->cv);
          if(n>0) n--; // decrease only if there is someone who is waiting

          pthread_mutex_unlock(arg->meOpen);

        }else{
          pthread_mutex_unlock(&(arg->meNodes[t->v]));
          #ifdef DEBUG
            printf("%d: g1=%d >= g(n')=%d, skip it!\n",arg->id, newGscore, neighboor_fScore - neighboor_hScore);
          #endif
          continue;
        }

      }else{
        pthread_mutex_lock(arg->meOpen);
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
          pthread_mutex_unlock(arg->meOpen);
          pthread_mutex_unlock(&(arg->meNodes[t->v]));
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
        pthread_mutex_unlock(arg->meOpen);
      }

      // change parent
      path[t->v] = extrNode.index;

      pthread_mutex_unlock(&(arg->meNodes[t->v]));

    }
  }

  printf("PANIC!!!!!!\n");
  exit(1);
}