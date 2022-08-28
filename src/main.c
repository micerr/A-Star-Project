#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include <time.h>
#include "Graph.h"
#include "AStar.h"
#include "Heuristic.h"
#include "PQ.h"
#define MAXC 11

int select_search_type(){
  int search_type;

  printf("Which kind of search do you want to perform?\n");
  printf("1 -> LINEAR SEARCH\n");
  printf("2 -> CONSTANT SEARCH\n");
  printf("3 -> PARALLEL SEARCH\n");
  printf("Enter your choice : ");

  if(scanf("%d", &search_type)<=0) {
      printf("Wrong input!\n");
      exit(0);
  }

  if((search_type <= 0) || (search_type > 3)){
    printf("Invalid option\n");
    exit(0);
  }

  return search_type;
}

int select_heuristic(){
  int heuristic;

  printf("Insert the heuristic function h(x) to use:\n");
  printf("\t1: Dijkstra emulator\n");
  printf("\t2: Euclidean distance\n");
  printf("\t3: Haversine formula\n");
  printf("\tEnter your choice : ");
  if(scanf("%d",&heuristic)<=0) {
    printf("Integers only!\n");
    exit(0);
  }

  return heuristic;
}

int main(void) {
  setbuf(stdout, NULL);
  
  int i, cont, id1, id2, search_type, numThreads;
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
    printf("7. Shortest path with HDA (Courirer Master)\n");
    printf("8. Shortest path with HDA (NO Courirer Master)\n");
    printf("9. Free graph and exit\n");
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
                        search_type = select_search_type();
                        GRAPHspD(G, id1, id2, search_type);
                        break;

            case 4:     printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);

                        i = select_heuristic();
                        search_type = select_search_type();

                        switch (i){
                          case 1:
                            ASTARSequentialAStar(G, id1, id2, Hdijkstra, search_type);
                            break;
                          case 2:
                            ASTARSequentialAStar(G, id1, id2, Hcoord, search_type);
                            break;
                          case 3:
                            ASTARSequentialAStar(G, id1, id2, Hhaver, search_type);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }

                        break;

            case 5:     printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);

                        i = select_heuristic();
                        search_type = select_search_type();

                        switch (i){
                          case 1:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hdijkstra, search_type);
                            break;
                          case 2:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hcoord, search_type);
                            break;
                          case 3:
                            ASTARSimpleParallel(G, id1, id2, numThreads, Hhaver, search_type);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                                                
                        break;

            case 6:     printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                                              
                        i = select_heuristic();
                        search_type = select_search_type();

                        switch (i){
                          case 1:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hdijkstra, search_type);
                            break;
                          case 2:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hcoord, search_type);
                            break;
                          case 3:
                            ASTARSimpleParallelV2(G, id1, id2, numThreads, Hhaver, search_type);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                                             
                        break;

            case 7:     if(G == NULL){
                          printf("No Graph inserted yet.\n");
                          break;
                        }
                        printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                        
                        i = select_heuristic();
                        search_type = select_search_type();

                        switch (i){
                          case 1:
                            ASTARhdaMaster(G, id1, id2, numThreads, Hdijkstra, search_type);
                            break;
                          case 2:
                            ASTARhdaMaster(G, id1, id2, numThreads, Hcoord, search_type);
                            break;
                          case 3:
                            ASTARhdaMaster(G, id1, id2, numThreads, Hhaver, search_type);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                                               
                        break;

            case 8:     if(G == NULL){
                          printf("No Graph inserted yet.\n");
                          break;
                        }
                        printf("\nInsert starting node = ");
                        scanf("%d", &id1);
                        printf("Insert destination node = ");
                        scanf("%d", &id2);
                        printf("Insert number of threads: ");
                        scanf("%d", &numThreads);
                        
                        i = select_heuristic();
                        search_type = select_search_type();

                        switch (i){
                          case 1:
                            ASTARhdaNoMaster(G, id1, id2, numThreads, Hdijkstra, search_type);
                            break;
                          case 2:
                            ASTARhdaNoMaster(G, id1, id2, numThreads, Hcoord, search_type);
                            break;
                          case 3:
                            ASTARhdaNoMaster(G, id1, id2, numThreads, Hhaver, search_type);
                            break;
                          default:
                            printf("\nInvalid option\n");
                            break;
                          }
                                               
                        break;

            case 9:    cont = 0;
                        break;
            default:    printf("\nInvalid option\n");
          }
        }
    }
    if(G == NULL) return 0;
    GRAPHfree(G);
    return 0;
}

// int main(void){
//   int correct_searches = 0;
//   int overall = 100;
//   setbuf(stdout, NULL);
//   PQ set = PQinit(100000000);


//   for(int i=0; i<100000000; i++){
//     PQinsert(set, i, i);
//   }

//   int prio, index;

//   //Timer timer = TIMERinit(1); 
//   srand(time(NULL));
//   int target;

//   //TIMERstart(timer);
//   for(int i=0; i<overall; i++){
//     target = rand() % 200000000;
//     index = PQsearch(set, target, &prio);
//     if(index == -1)
//       printf("%d - Target not found: target=%d \n", i, target);
//     else
//       printf("%d - Result: target=%d index=%d, priority=%d\n", i, target, index,prio);
//     if((index == target) && (index == prio)){
//       correct_searches += 1;
//     }
//     else if((index == -1) && (target > 100000000)){
//       correct_searches += 1;
//     }

//   }

//   printf("\nCorrect searches: %d over %d\n", correct_searches, overall);
// }

