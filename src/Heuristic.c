#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Heuristic.h"

#define R 6378.137

/*  
    The haversine formula determines the great-circle distance between two points on a sphere given their longitudes and latitudes.
    https://stackoverflow.com/questions/639695/how-to-convert-latitude-or-longitude-to-meters
    https://en.wikipedia.org/wiki/Haversine_formula

    Return: in meters the distance of two points 
*/
static double Harversine(Coord coord1, Coord coord2){
    double dLat = (coord2->lat * M_PI / 180 - coord1->lat * M_PI /180) / 2;
    double dLon = (coord2->lon * M_PI / 180 - coord1->lon * M_PI /180) / 2;
    double a = sin(dLat);
    double b = sin(dLon);
    double c = a * a + cos(coord1->lat * M_PI / 180) * cos(coord2->lat * M_PI / 180) * b * b;
    double d = 2 * atan2(sqrt(c), sqrt(1-c));
    return R * d * 1000; // meters
}

double Hcoord(Coord coord1, Coord coord2){
    return Harversine(coord1, coord2);
}

