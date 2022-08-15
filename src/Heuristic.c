#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Heuristic.h"

double Hcoord(Coord coord1, Coord coord2){
    return sqrt(pow(coord1->c1 - coord2->c1, 2) + pow(coord1->c2 - coord2->c2, 2));
}
