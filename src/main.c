#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "AStar.h"
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
    printf("5. Shortest path with Dijkstra's algorithm\n");
    printf("6. Shortest path with sequential A*\n");
    printf("7. Free graph and exit\n");
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

            case 5:     printf("\nInsert start node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHspD(G, id1, id2);
                        break;

            case 6:     printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        GRAPHSequentialAStar(G, id1, id2);
                        break;

            case 7:     cont = 0;
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
