#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "AStar.h"
#include "Heuristic.h"
#define MAXC 11

int main(void) {
  int i, cont, id1, id2, wt, numThreads;
  char name[MAXC];
  Graph G = NULL;

  cont = 1;
  while(cont) {
    printf("\nOperations on weighted directed graphs\n");
    printf("===============\n");
    printf("1. Load graph from file (sequential)\n");
    printf("2. Load graph from file (parallel)\n");
    printf("3. Edge insertion\n");
    printf("4. Edge removal\n");
    printf("5. Get coordinates of a vertex\n");
    printf("6. Compute distance\n");
    printf("7. Get weight of an edge\n");
    printf("8. Check Admissibility\n");
    printf("---\n");
    printf("9. Shortest path with Dijkstra's algorithm\n");
    printf("10. Shortest path with sequential A*\n");
    printf("11. Free graph and exit\n");
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
                        printf("Insert second node = ");
                        scanf("%d", &id2);
                        printf("Insert weight = ");
                        scanf("%d", &wt);
                        GRAPHinsertE(G, id1, id2, wt);
                        break;

            case 4:     printf("\nInsert first node = ");
                        scanf("%d", &id1);
                        printf("Insert second node = ");
                        scanf("%d", &id2);
                        GRAPHremoveE(G, id1, id2);
                        break;
                    
            case 5:     printf("\nInsert node's index = ");
                        scanf("%d", &id1);
                        GRAPHgetCoordinates(G, id1);
                        break;

            case 6:     printf("\nInsert first node = ");
                        scanf("%d", &id1);
                        printf("Insert second node = ");
                        scanf("%d", &id2);
                        GRAPHcomputeDistance(G, id1, id2);
                        break;

            case 7:     printf("\nInsert start node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHgetEdge(G, id1, id2);
                        break;

            case 8:     printf("\nInsert first node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHcheckAdmissibility(G, id1, id2);
                        break;

            case 9:     printf("\nInsert first node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHspD(G, id1, id2);
                        break;

            case 10:    printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert the heuristic function h(x) to use:\n");
                        printf("1: Dijkstra emulator\n");
                        printf("2: Euclidean distance\n");
                        printf("3: Haversine formula\n");
                        printf("Enter your choice : ");
                        if(scanf("%d",&i)<=0) {
                          printf("Integers only!\n");
                          exit(0);
                        }else{
                          switch (i)
                          {
                          case 1:
                            GRAPHSequentialAStar(G, id1, id2, Hdijkstra);
                            break;
                          case 2:
                            GRAPHSequentialAStar(G, id1, id2, Hcoord);
                            break;
                          case 3:
                            GRAPHSequentialAStar(G, id1, id2, Hhaver);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                        }
                        
                        break;

            case 11:    cont = 0;
                        break;
            /*TO BE REMOVED*/
            case 8:
                        printf("Starting parallel search test\n");
                        GRAPHParallelSearchTest();
                        break;
            /* END */
            default:    printf("\nInvalid option\n");
          }
        }
    }
    if(G == NULL) return 0;
    GRAPHfree(G);
    return 0;
}
