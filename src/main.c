#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include "Graph.h"
#include "AStar.h"
#include "Heuristic.h"
#define MAXC 11

int main(void) {
  int i, cont, id1, id2, wt, numThreads;
  int index, priority, father;                      
  char name[MAXC];
  Graph G = NULL;
  Queue queue = NULL;

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
    printf("100. Queue - Insert node .\n");
    printf("101. Queue - Extract node.\n");
    printf("102. Queue - Print queue.\n");
    printf("103. Queue - 100 elements and print.\n");
    printf("104. Queue - Free queue.\n");
    printf("---\n");
    printf("9. Shortest path with Dijkstra's algorithm\n");
    printf("10. Shortest path with sequential A*\n");
    printf("11. Shortest path with SPA*\n");
    printf("12. Free graph and exit\n");
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

            case 11:    printf("\nInsert starting node = ");
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

            case 12:    cont = 0;
                        break;

            case 100:   if(queue == NULL)
                          queue = QUEUEinit();
                        printf("\tInsert the index: ");
                        scanf("%d", &index);
                        printf("\tInsert the priority: ");
                        scanf("%d", &priority);
                        printf("\tInsert the father: ");
                        scanf("%d", &father);

                        QUEUEtailInsert(queue, HITEMinit(index, priority, father, NULL));
                        break;
            
            case 101:   if(queue == NULL){
                          printf("Queue is not initialized.\n");
                          break;
                        }
                        HItem_ptr t;
                        t = QUEUEheadExtract(queue);
                        if(t == NULL){
                          printf("Nothing extracted.\n");
                          break;
                        }

                        printf("Extracted node:\n");
                        printf("\tindex: %d\n", t->index);
                        printf("\tpriority: %d\n", t->priority);
                        printf("\tfather: %d\n", t->father);
                        break;

            case 102:   if(queue == NULL){
                          printf("Queue is not initialized.\n");
                          break;
                        }

                        QUEUEprint(queue);
                        break;

            case 103:   if(queue != NULL){
                          printf("One queue was already present.\nI'm going to free it.\n");
                          QUEUEfree(queue);
                        }

                        queue = QUEUEinit();

                        for(int i=0; i<100; i++)
                          QUEUEtailInsert(queue, HITEMinit(i,i,i,NULL));
                        QUEUEprint(queue);
                        break;

            case 104:   if(queue == NULL){
                          printf("Queue is not initialized.\n");
                          break;
                        }

                        QUEUEfree(queue);
                        queue = NULL;
                        break;

            default:    printf("\nInvalid option\n");
          }
        }
    }
    if(G == NULL) return 0;
    GRAPHfree(G);
    return 0;
}
