#ifndef TEST_H
#define TEST_H

#include <sys/time.h>

#include "Graph.h"

typedef struct analytics_s *Analytics;

Analytics   ANALYTICSsave(Graph G, int start, int end, int *path, int cost, int numExtr,int maxNodeAssigned, float avgNodeAssigned, float comOverhead, double algorithmTime, int byte);
void        ANALYTICSfree(Analytics);

#endif