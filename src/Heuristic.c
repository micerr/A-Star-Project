#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Heuristic.h"

double Hcoord(Coord coord1, Coord coord2){
    printf("Coord1 -> c1: %d, c2: %d\n", coord1->c1, coord1->c2);
    printf("Coord2 -> c1: %d, c2: %d\n", coord2->c1, coord2->c2);

    return sqrt(pow(coord1->c1 - coord2->c1, 2) + pow(coord1->c2 - coord2->c2, 2));
}

