#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

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
void GRAPHSequentialAStar(Graph G, int start, int end){
  if(G == NULL){
    printf("No graph inserted.\n");
    return ;
  }

  int v, hop=0;
  ptr_node t;
  Item extrNode;
  PQ openSet;
  int *path, *closedSet;
  int *gScore, *fScore;
  int newGscore, newFscore;
  int flag;
  Coord coord, dest_coord;

  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

  openSet = PQinit(1);

  path = malloc(G->V * sizeof(int));
  if(path == NULL){
    exit(1);
  }

  gScore = malloc(G->V * sizeof(int));
  fScore = malloc(G->V * sizeof(int));
  closedSet = malloc(G->V * sizeof(int));
  
  //insert all nodes in the priority queue with total weight equal to infinity
  for (v = 0; v < G->V; v++){
    path[v] = -1;
    closedSet[v] = 0;
    gScore[v] = fScore[v] = maxWT;
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

  coord = STsearchByIndex(G->coords, start);
  dest_coord = STsearchByIndex(G->coords, end);

  path[start] = start;
  gScore[start] = 0;
  fScore[start] = gScore[start] + Hcoord(coord, dest_coord);

  PQinsert(openSet, start, 0);

  while (!PQempty(openSet)){
    //extract node
    extrNode = PQextractMin(openSet);
    //printf("extrNode.index: %d\n", extrNode.index);
    #if DEBUG
      printf("Min extracted is (index): %d\n", extrNode.index);
    #endif

    //add to the closed set
    closedSet[extrNode.index] = 1;

    //it it is the destination node, end computation
    if(extrNode.index == end){
      // we reached the end node
      break;
    }

    coord = STsearchByIndex(G->coords, extrNode.index);

    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      newGscore = gScore[extrNode.index] + t->wt;
      newFscore = newGscore + Hcoord(coord, dest_coord);

      //if it belongs to the closed set
      if(closedSet[t->v] == 1){
        if(newGscore < gScore[t->v]){
          closedSet[t->v] = 0;
          //printf("%d is changing priority of %d (f(n)=%d)\n", extrNode.index, t->v, newFscore);
          PQchange(openSet, t->v, newFscore);
        }
        else
          continue;
      }else{  //it doesn't belong to closed set
        flag = PQsearch(openSet, t->v, NULL);

        //if it doesn't belong to the open set
        if(flag < 0){
          PQinsert(openSet, t->v, newFscore);
          //printf("%d is adding %d (f(n)=%d)\n", extrNode.index, t->v, newFscore);
        }
        
        else if(newGscore >= gScore[t->v])
          continue;
        else
          PQchange(openSet, t->v, newFscore);
      }
      gScore[t->v] = newGscore;
      fScore[t->v] = newFscore;
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
    printf("Path from %d to %d has been found with cost %.3f.\n", start, end, extrNode.priority);
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
  free(gScore);
  free(fScore);

  return;
}