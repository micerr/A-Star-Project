#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#define MAXC 11

int main(void) {
  int i, cont, id1, id2, wt;
  char name[MAXC];
  Graph G;

  cont = 1;
  while(cont) {
    printf("\nOperations on weighted directed graphs\n");
    printf("===============\n");
    printf("1.Load graph from file\n");
    printf("2.Edge insertion\n");
    printf("3.Edge removal\n");
    printf("4.Store graph to file\n");
    printf("5.Shortest path with Dijkstra's algorithm\n");
    printf("6.Free graph and exit\n");
    printf("Enter your choice : ");
    if(scanf("%d",&i)<=0) {
      printf("Integers only!\n");
      exit(0);
    }
    else {
      switch(i) {
            case 1:     printf("Input file name: ");
                        scanf("%s", name);
                        G = GRAPHload(name);
                        break;
            case 2:     printf("Insert first node = ");
                        scanf("%d", &id1);
                        printf("Insert second node = ");
                        scanf("%d", &id2);
                        printf("Insert weight = ");
                        scanf("%d", &wt);
                        GRAPHinsertE(G, id1, id2, wt);
                        break;
            case 3:     printf("Insert first node = ");
                        scanf("%d", &id1);
                        printf("Insert second node = ");
                        scanf("%d", &id2);
                        GRAPHremoveE(G, id1, id2);
                        break;
            case 4:     printf("Input file name: ");
                        scanf("%s", name);
                        GRAPHstore(G, name);
                        break;
            case 5:     printf("Insert start node = ");
                        scanf("%d", &id1);
                        GRAPHspD(G, id1);
                        break;
            case 6:     cont = 0;
                        break;
            default:    printf("Invalid option\n");
          }
        }
    }
    GRAPHfree(G);
    return 0;
}
