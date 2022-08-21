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
static double Haversine(Coord coord1, Coord coord2){
    double c11 = coord1->c1 / 1000000.0;
    double c12 = coord1->c2 / 1000000.0;
    double c21 = coord2->c1 / 1000000.0;
    double c22 = coord2->c2 / 1000000.0;

    double dLat = (c21 * M_PI / 180 - c11 * M_PI /180) / 2;
    double dLon = (c22 * M_PI / 180 - c12 * M_PI /180) / 2;
    double a = sin(dLat);
    double b = sin(dLon);
    double c = a * a + cos(c11 * M_PI / 180) * cos(c21 * M_PI / 180) * b * b;
    double d = 2 * atan2(sqrt(c), sqrt(1-c));
    return R * d * 1000; // meters
}

int Hcoord(Coord coord1, Coord coord2){
    return sqrt(pow(coord1->c1 - coord2->c1,2) + pow(coord1->c2 - coord2->c2,2));
}

int Hhaver(Coord coord1, Coord coord2){
    return Haversine(coord1, coord2);
}

int Hdijkstra(Coord coord1, Coord coord2){
    return 0;
}
