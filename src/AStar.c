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

// Data structures:
// openSet -> Priority queue containing an heap made of Items (Item has index of node and priority (fScore))
// closedSet -> BitArray: 1 if node expanded, 0 otherwise
// path -> the path of the graph (built step by step)
// fScores are embedded within an Item 
// gScores are obtained starting from the fScores and subtracting the value of the heuristic

//openSet is enlarged gradually (inside PQinsert)

PQ openSet_PQ;
int *closedSet, *path;

typedef struct thArg_s {
  pthread_t tid;
  Graph G;
  int start, end, numTH;
} thArg_t;

void ASTARSimpleParallel(Graph G, int start, int end, int numTH){
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }
  float prio;
  Coord dest_coord, coord;
  thArg_t *thArgArray;

  //init the open set (priority queue)
  openSet_PQ = PQinit(1);
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

  //create and launch all threads
  thArgArray = (thArg_t *)malloc(numTH * sizeof(thArg_t));
  if(thArgArray == NULL){
    perror("Error trying to allocate the array of threads' arguments: ");
    exit(1);
  }

  for(int i=0; i<numTH; i++){
    thArgArray[i].G = G;
    thArgArray[i].start = start;
    thArgArray[i].end = end;
    thArgArray[i].numTH = numTH;
    pthread_create(thArgArray[i].tid, NULL, thFunction, (void *)&thArgArray[i]);
  }

  for(int i=0; i<numTH; i++)
    pthread_join(thArgArray[i].tid, NULL);

  //printf path and its cost...

  return;
}

void thFunction(void *par){
  thArg_t *arg = (thArg_t *)par;

  Item extrNode;
  Coord extr_coord;

  //until the open set is not empty
  while(!PQempty(openSet_PQ)){

    //extract the vertex with the lowest fScore
    extrNode = PQextractMin(openSet_PQ);

    //retrieve its coordinates
    extr_coord = STsearchByIndex(G->coords, extrNode.index);
    
    //add the extracted node to the closed set
    closedSet[extrNode.index] = extrNode.priority;

    //if the extracted node is the goal one, end the computation
    if(extrNode.index == end)
      break;

    //consider all adjacent vertex of the extracted node
    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      //retrieve coordinates of the adjacent vertex
      neighboor_coord = STsearchByIndex(G->coords, t->v);
      neighboor_hScore = Hcoord(neighboor_coord, dest_coord);

      //cost to reach the extracted node is equal to fScore less the heuristic.
      //newGscore is the sum between the cost to reach extrNode and the
      //weight of the edge to reach its neighboor.
      newGscore = (extrNode.priority - Hcoord(coord, dest_coord)) + t->wt;

      //check if adjacent node has been already closed
      if(closedSet[t->v] > 0){
        neighboor_fScore = closedSet[t->v];
        neighboor_gScore = neighboor_fScore - neighboor_hScore;

        //if a lower gScore is found, reopen the vertex
        if(newGscore < neighboor_gScore){
          //remove it from closed set
          closedSet[t->v] = -1;
          
          //add it to the open set
          newFscore = newGscore + neighboor_hScore;
          PQinsert(openSet_PQ, t->v, newFscore);
          path[t->v] = extrNode.index;
        }
        else
          continue;
      }
      //if it hasn't been closed yet
      else{
        flag = PQsearch(openSet_PQ, t->v, &neighboor_fScore);
        neighboor_gScore = neighboor_fScore - neighboor_hScore;

        //if it doesn't belong to the open set yet, add it
        if(flag<0){
          newFscore = newGscore + neighboor_hScore;
          PQinsert(openSet_PQ, t->v, newFscore);
          path[t->v] = extrNode.index;
        }
        //if it belongs to the open set but with a lower gScore, continue
        else if(neighboor_gScore < newGscore)
          continue;
        else{
          newFscore = newGscore + neighboor_hScore;
          PQchange(openSet_PQ, t->v, newFscore);
          path[t->v] = extrNode.index;
        }
      }
    }
  }

  if(closedSet[end] < 0)
    printf("No path from %d to %d has been found.\n", start, end);
  else{
    printf("Path from %d to %d has been found with weight %.3f.\n", start, end, extrNode.priority);
    for(int v=end; v!=start; ){
      printf("%d <- ", v);
      v = path[v];
    }
    printf("%d\n",start);
  }

  //print path cost ...


  return;
}
