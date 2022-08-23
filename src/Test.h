#ifndef TEST_H
#define TEST_H

#include <sys/time.h>

#include "Graph.h"

typedef struct analytics_s *Analytics;

Analytics   ANALYTICSsave(Graph G, int start, int end, int *path, int cost, double algorithmTime);
void        ANALYTICSfree(Analytics);

#endif