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

char spinner[] = "|/-\\";
int spin = 0;

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
    int expandedNodes = 0;
    printf("sizeofPath= %d B (%d MB), sizeofPQ= %d B (%d MB), TOT= %d B (%d MB)\n", sizeofPath, sizeofPath>>20, sizeofPQ, sizeofPQ>>20, total, total>>20);
    for(int i=0;i<G->V;i++){
      if(path[i]>=0)
        expandedNodes++;
    }
    printf("Expanded nodes: %d\n",expandedNodes);
  #endif

  // Print the found path
  if(path[end] == -1){
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
  if(C == -1) return -1;
  int h = Hdijkstra(STsearchByIndex(G->coords, source), coordTarget);
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
void GRAPHSequentialAStar(Graph G, int start, int end, double (*h)(Coord, Coord)){
  if(G == NULL){
    printf("No graph inserted.\n");
    return;
  }

  PQ openSet_PQ;
  float *closedSet;
  int *path, hop=0, isNotConsistent = 0, isInsert;
  float newGscore, newFscore, prio;
  float neighboor_fScore, neighboor_hScore;
  Item extrNode;
  Coord dest_coord, coord, neighboor_coord;
  ptr_node t;
  #ifdef TIME
    Timer timer = TIMERinit(1);
  #endif

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

  //allocate the path array
  path = (int *) malloc(G->V * sizeof(int));
  if(path == NULL){
    perror("Error trying to allocate path array: ");
    exit(1);
  }

  for(int i=0; i<G->V; i++){
    closedSet[i]=-1;
    path[i]=-1;
  }

  #ifdef TIME
    TIMERstart(timer);
  #endif
  
  //retrieve coordinates of the target vertex
  dest_coord = STsearchByIndex(G->coords, end);

  //compute starting node's fScore. (gScore = 0)
  coord = STsearchByIndex(G->coords, start);
  prio = h(coord, dest_coord);

  //insert the starting node in the open set (priority queue)
  PQinsert(openSet_PQ, start, prio);
  path[start] = start;

  //until the open set is not empty
  while(!PQempty(openSet_PQ)){
    printf("%c\b", spinner[(spin++)%4]);

    //extract the vertex with the lowest fScore
    extrNode = PQextractMin(openSet_PQ);

    //retrieve its coordinates
    coord = STsearchByIndex(G->coords, extrNode.index);
    
    //add the extracted node to the closed set
    closedSet[extrNode.index] = extrNode.priority;

    //if the extracted node is the goal one, end the computation
    if(extrNode.index == end)
      break;

    //consider all adjacent vertex of the extracted node
    for(t=G->ladj[extrNode.index]; t!=G->z; t=t->next){
      //retrieve coordinates of the adjacent vertex
      neighboor_coord = STsearchByIndex(G->coords, t->v);
      neighboor_hScore = h(neighboor_coord, dest_coord);

      //cost to reach the extracted node is equal to fScore less the heuristic.
      //newGscore is the sum between the cost to reach extrNode and the
      //weight of the edge to reach its neighboor.
      newGscore = (extrNode.priority - h(coord, dest_coord)) + t->wt;
      newFscore = newGscore + neighboor_hScore;

      isInsert = -1;

      //check if adjacent node has been already closed
      if( (neighboor_fScore = closedSet[t->v]) >= 0){
        // n' belongs to CLOSED SET

        //if a lower gScore is found, reopen the vertex
        if(newGscore < neighboor_fScore - neighboor_hScore){
          if(!isNotConsistent){
            printf("The heuristic function is NOT consistent\n");
            isNotConsistent = 1;
          }
          //remove it from closed set
          closedSet[t->v] = -1;
          
          //add it to the open set
          isInsert = 1;
        }
        else
          continue;
      }
      //if it hasn't been closed yet
      else{
        if(PQsearch(openSet_PQ, t->v, &neighboor_fScore) < 0){
          //if it doesn't belong to the open set yet, add it
          isInsert = 1;
        }
        //if it belongs to the open set but with a lower gScore, continue
        else if(newGscore >= neighboor_fScore - neighboor_hScore)
          continue;
        else{
          // change the fScore if this new path is better
          isInsert = 0;
        }
      }

      if(isInsert == -1){
        // error
        printf("PANIC!!!!!!\n");
        exit(1);
      }else if(isInsert){
        // PQinsert new Fscore
        PQinsert(openSet_PQ, t->v, newFscore);
      }else{
        // PQchange new Fscore
        PQchange(openSet_PQ, t->v, newFscore);
      }
      // change parent
      path[t->v] = extrNode.index;
    }
  }

  #ifdef TIME
    TIMERstopEprint(timer);
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

  if(closedSet[end] < 0)
    printf("No path from %d to %d has been found.\n", start, end);
  else{
    printf("Path from %d to %d has been found with cost %.3f.\n", start, end, extrNode.priority);
    for(int v=end; v!=start; ){
      printf("%d <- ", v);
      v = path[v];
      hop++;
    }
    printf("%d\nHops: %d\n", start, hop);
  }


  #ifdef TIME
    TIMERfree(timer);
  #endif
  free(path);
  free(closedSet);
  PQfree(openSet_PQ);

  return;
}