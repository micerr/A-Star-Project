#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include "Graph.h"
#include "AStar.h"
#include "Heuristic.h"
#define MAXC 11

int main(void) {
  int i, cont, id1, id2, numThreads;
  int index, priority, father;                      
  char name[MAXC];
  Graph G = NULL;

  cont = 1;
  while(cont) {
    printf("\nOperations on weighted directed graphs\n");
    printf("===============\n");
    printf("1. Load graph from file (sequential)\n");
    printf("2. Load graph from file (parallel)\n");
    printf("---\n");
    printf("3. Shortest path with Dijkstra algorithm\n");
    printf("4. Shortest path with sequential A*\n");
    printf("5. Shortest path with SPA*\n");
    printf("6. Shortest path with SPA* Version 2\n");
    printf("7. Free graph and exit\n");
    printf("===============\n");
    printf("Enter your choice : ");
    if(scanf("%d",&i)<=0) {
      printf("Integers only!\n");
      exit(0);
    }
    else {
      switch(i) {
            case 1:     //take path of the input file
                        printf("\nInput file name: ");
                        scanf("%s", name);
                        //if another graph already exists, free it before
                        if(G != NULL)
                          GRAPHfree(G);
                        //call function to sequentially load the graph
                        G = GRAPHSequentialLoad(name);
                        break;
                
            case 2:     //take path of the input file
                        printf("\nInput file name: ");
                        scanf("%s", name);
                        //take number of thread to launch
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                        //if another graph already exists, free it before
                        if(G != NULL)
                          GRAPHfree(G);
                        //call the function to load the graph in a parallel fashion
                        G = GRAPHParallelLoad(name, numThreads);
                        break;

            case 3:     printf("\nInsert first node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHspD(G, id1, id2);
                        break;

            case 4:     printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert the heuristic function h(x) to use:\n");
                        printf("\t1: Dijkstra emulator\n");
                        printf("\t2: Euclidean distance\n");
                        printf("\t3: Haversine formula\n");
                        printf("\tEnter your choice : ");
                        if(scanf("%d",&i)<=0) {
                          printf("Integers only!\n");
                          exit(0);
                        }else{
                          switch (i)
                          {
                          case 1:
                            ASTARSequentialAStar(G, id1, id2, Hdijkstra);
                            break;
                          case 2:
                            ASTARSequentialAStar(G, id1, id2, Hcoord);
                            break;
                          case 3:
                            ASTARSequentialAStar(G, id1, id2, Hhaver);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                        }
                        break;

            case 5:    printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                        printf("Insert the heuristic function h(x) to use:\n");
                        printf("\t1: Dijkstra emulator\n");
                        printf("\t2: Euclidean distance\n");
                        printf("\t3: Haversine formula\n");
                        printf("\tEnter your choice : ");
                        if(scanf("%d",&i)<=0) {
                          printf("Integers only!\n");
                          exit(0);
                        }else{
                          switch (i)
                          {
                          case 1:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hdijkstra);
                            break;
                          case 2:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hcoord);
                            break;
                          case 3:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hhaver);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                        }                        
                        break;

            case 6:    printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                        printf("Insert the heuristic function h(x) to use:\n");
                        printf("\t1: Dijkstra emulator\n");
                        printf("\t2: Euclidean distance\n");
                        printf("\t3: Haversine formula\n");
                        printf("\tEnter your choice : ");
                        if(scanf("%d",&i)<=0) {
                          printf("Integers only!\n");
                          exit(0);
                        }else{
                          switch (i)
                          {
                          case 1:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hdijkstra);
                            break;
                          case 2:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hcoord);
                            break;
                          case 3:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hhaver);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                        }                        
                        break;

            case 7:    cont = 0;
                        break;
            default:    printf("\nInvalid option\n");
          }
        }
    }
    if(G == NULL) return 0;
    GRAPHfree(G);
    return 0;
}
