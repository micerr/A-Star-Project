#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "Test.h"
#include "Graph.h"
#include "AStar.h"
#include "Heuristic.h"
#include "PQ.h"
#include "./utility/Timer.h"

#define P 10
#define maxNameAlg 20
#define maxNameHash 12
#define nSeq 2

struct analytics_s{
    int *path, len, cost, expandedNodes, numExtr;
    double loadBalance, co;
    double totTime, algorithmTime;
    struct timeval endTotTime;
};

struct algorithms_s{
    Analytics (*algorithm)(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type);
    Analytics (*hda)(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type, int (*hfunc)(Hash h, int v));
    char name[maxNameAlg+maxNameHash];
    int isConcurrent;
} algorithms[] = {
    {ASTARSequentialAStar, NULL, "A* Dijkstra", 0},
    {ASTARSequentialAStar, NULL, "A*", 0},
    {ASTARSimpleParallel, NULL, "SPA* single-mutex", 1},
    {ASTARSimpleParallelV2, NULL, "SPA* multiple-mutex", 1},
    {NULL, ASTARhdaMaster, "HDA w Master", 2},
    {NULL, ASTARhdaNoMaster, "HDA", 2},
    {NULL, NULL, "", -1}
};

struct hashF_s{
    void* hash;
    char name[maxNameHash];
} hashF[] = {
    {abstractFeatureZobristHashing, "A{feature}Z"},
    {abstractStateZobristHashing, "A{state}Z"},
    {zobristHashing, "Z"},
    {multiplicativeHashing, "mult"},
    {NULL, ""}
};

int threads[]={
    1, 2, 3, 4, 5, 6, 8, 12, 16, -1
};

typedef struct point_s{
    int src, dst;
}Point;

static void* genPoint(Point* points, int n, int maxV);
static void printAnalytics(char *name, int isConcurrent, int numTh, Analytics, int **correctPath, int *lenCorrect, int *expandedSeq);
static void* select_heuristic();

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "Error: no file inserted. <filePath>\n");
        exit(1);
    }

    Analytics stats;
    struct timeval begin;
    int startFrom, i, numThreads;
    Graph G;

    printf("Do the nodes start from 0 or 1?\n");
    printf("Enter your choice: ");
    if(scanf("%d", &startFrom)<=0) {
        printf("Wrong input!\n");
        exit(0);
    }

    // Loading graph
    printf("1. Load graph from file (sequential)\n");
    printf("2. Load graph from file (parallel)\n");
    if(scanf("%d",&i)<=0) {
      printf("Integers only!\n");
      exit(0);
    }
    if(i==1){
        G = GRAPHSequentialLoad(argv[1], startFrom);
        //G = GRAPHSequentialLoad("../examples/01-NY/01-NY-distanceWeight-new.bin", 1);
    }else if(i==2){
        printf("Insert number of threads: ");
        scanf("%d", &numThreads);
        G = GRAPHParallelLoad(argv[1], numThreads, startFrom);
    }else{
        printf("\nInvalid option\n");
        exit(1);
    }

    int (*heuristic)(Coord, Coord) = select_heuristic();


    // Allocate 10 points, in which we will do tests
    Point *points =(Point *) malloc(P*sizeof(Point));

    genPoint(points, P, G->V);

    for(int i=0; i<P; i++){
        int *correctPath = NULL, correctLen = -1, expandedSeq = -1;

        begin = TIMERgetTime();

        stats = GRAPHspD(G, points[i].src, points[i].dst, CONSTANT_SEARCH);
        stats->totTime = computeTime(begin, stats->endTotTime);

        int distance = Hhaver(STsearchByIndex(G->coords, points[i].src), STsearchByIndex(G->coords, points[i].dst));
        printf("Test on path (%d, %d), cost: %d, hops: %d, crow flies distance: %dm\n", points[i].src, points[i].dst, stats->len != 0 ? stats->cost : -1, stats->len-1, distance);
        printf("%-26s%-10s%-10s%-7s%-13s%-17s%-17s%-18s%-25s%-18s%-15s%-6s\n","Algorithm","Threads","Cost", "Hops","Total Time", "Algorithm Time", "Expanded Nodes", "Extracted Nodes","Communication Overhead", "Search Overhead", "Load Balance", "PASSED");
        printf("---------------------------------------------------------------------------------------------\n");

        printAnalytics("Dijkstra", 0, 1, stats, &correctPath, &correctLen, &expandedSeq);

        for(int j=0; algorithms[j].isConcurrent != -1; j++){
            if(!algorithms[j].isConcurrent){
                // sequential algorithm
                begin = TIMERgetTime();
                

                stats = algorithms[j].algorithm(G, points[i].src, points[i].dst, 1, strcmp(algorithms[j].name, "A* Dijkstra")==0 ? Hdijkstra : heuristic, 2);
                stats->totTime = computeTime(begin, stats->endTotTime);
                
                printAnalytics(algorithms[j].name, algorithms[j].isConcurrent, 1, stats, &correctPath, &correctLen, &expandedSeq);
                ANALYTICSfree(stats);
            }else{
                // concurrent algorithms SPA
                for(int k=0; threads[k]!=-1 && algorithms[j].isConcurrent == 1; k++){
                    if(threads[k]>10)
                        break;
                    begin = TIMERgetTime();

                    stats = algorithms[j].algorithm(G, points[i].src, points[i].dst, threads[k], heuristic, 2);
                    stats->totTime = computeTime(begin, stats->endTotTime);

                    printAnalytics(algorithms[j].name, algorithms[j].isConcurrent, threads[k], stats, &correctPath, &correctLen, &expandedSeq);
                    ANALYTICSfree(stats);
                }
                // concurrent algorithms HDA
                for(int n=0; hashF[n].hash!=NULL && algorithms[j].isConcurrent == 2; n++){
                    for(int k=0; threads[k]!=-1; k++){
                        if(hashF[n].hash == randomHashing && threads[k]>4)
                            break;
                        begin = TIMERgetTime();

                        stats = algorithms[j].hda(G, points[i].src, points[i].dst, threads[k], heuristic, 2, hashF[n].hash);
                        stats->totTime = computeTime(begin, stats->endTotTime);

                        char name[64];
                        sprintf(name,"%s%s", hashF[n].name, algorithms[j].name);

                        printAnalytics(name, algorithms[j].isConcurrent, threads[k], stats, &correctPath, &correctLen, &expandedSeq);
                        ANALYTICSfree(stats);
                    }
                }

            }
        }
        if(correctPath != NULL)
            free(correctPath);
        printf("---------------------------------------------------------------------------------------------\n");
        printf("\n");
    }

    // deallocation
    free(points);
    GRAPHfree(G);

    return 0;
}

/*
    Print all the statistics
*/
static void printAnalytics(char *name, int isConcurrent, int numTh, Analytics stats, int **correctPath, int *lenCorrect, int *expandedSeq){
		int ok = 1;

        //checks if passed or not
		if(strcmp(name, "Dijkstra") == 0){
            *lenCorrect = stats->len;
            if(stats->len != 0){
                *correctPath = malloc(stats->len*sizeof(int));
			    for(int i=0; i<stats->len; i++){
			    	(*correctPath)[i] = stats->path[i];
			    }
            }
		}else if(*correctPath != NULL){
            if(strcmp(name, "A*")==0){
                *expandedSeq = stats->expandedNodes;
            }
            // checks the lenght of the path
            if(stats->len != *lenCorrect){
                ok = 0;
            }else{
                // checks the paths
			    for(int i=0; i<stats->len; i++){
			    	if((*correctPath)[i] != stats->path[i]){
			    		ok = 0;
                        break;
			    	}
			    }
            }
		}

    printf("%-26s%-10d", numTh==1 ? name : "", numTh);
    if(stats->len != 0)
        printf("%-10d", stats->cost);
    else
        printf("%-10s", "no path");
    printf("%-7d", stats->len-1);
    if(stats->algorithmTime < 0.010)
        printf("%-13.6f%-17.6f",stats->totTime, stats->algorithmTime);
    else
        printf("%-13.3f%-17.3f",stats->totTime, stats->algorithmTime);
    printf("%-17d%-18d", stats->expandedNodes, stats->numExtr);
    if(isConcurrent > 0){
        printf("%-25.3f",stats->co);
        printf("%-18.3f",(stats->expandedNodes/(*expandedSeq))-1.0);
        printf("%-15.3f",stats->loadBalance);
    }else
        printf("%-25s%-18s%-15s","-","-","-");
    printf("%-6s\n",ok ? "OK" : "NO");
    return;
}

/*
    Generate n points and put them inside *points
*/
static void* genPoint(Point* points, int n, int maxV){
    if(points == NULL)
        return NULL;

    struct timeval now = TIMERgetTime();

    srand(now.tv_sec+now.tv_usec);

    for(int i=0; i<n; i++){
        points[i].src = rand()/((RAND_MAX + 1u)/maxV);
        points[i].dst = rand()/((RAND_MAX + 1u)/maxV);
    }

    return points;
}

Analytics ANALYTICSsave(Graph G, int start, int end, int *path, int cost, int numExtr, int maxNodeAssigned, float avgNodeAssigned, float comOverhead, double algorithmTime){
    struct timeval now = TIMERgetTime();

    Analytics stats = malloc(sizeof(struct analytics_s));

    stats->numExtr = numExtr;
    stats->endTotTime = now;
    stats->cost = cost;
    stats->algorithmTime = algorithmTime;
    stats->len = 0;
    stats->co = comOverhead;
    if(avgNodeAssigned != 0)
        stats->loadBalance = maxNodeAssigned/avgNodeAssigned;
    // compute number of expanded nodes
	stats->expandedNodes = 0;
	for(int i=0; i<G->V; i++)
		if(path[i] >= 0)
			stats->expandedNodes++;
    // check if no path is found
    if(path[end] == -1)
        return stats;
    // compute the len
    for(int i=end; i!=start; i=path[i]){
      stats->len++;
    }
    // allocate the vector
    stats->path = malloc(stats->len*sizeof(int));
    int i = stats->len-1;
    // save the new path
    for(stats->path[i]=end; i>0; i--){
      stats->path[i-1] = path[stats->path[i]];
    }
    return stats;
}

void ANALYTICSfree(Analytics stats){
    if(stats != NULL && stats->path != NULL && stats->len != 0) 
        free(stats->path);
    if(stats != NULL)
        free(stats);
    return;
}

static void* select_heuristic(){
  int heuristic;

  printf("Insert the heuristic function h(x) to use:\n");
  printf("\t1: Euclidean distance\n");
  printf("\t2: Haversine formula\n");
  printf("\tEnter your choice : ");
  if(scanf("%d",&heuristic)<=0) {
    printf("Integers only!\n");
    exit(0);
  }

  switch (heuristic){
    case 1:
        return Hcoord;
    case 2:
        return Hhaver;
    default:
        printf("\nInvalid option\n");
        exit(1);
  }
}