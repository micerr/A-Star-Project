#ifndef ST_H
#define ST_H

typedef struct coord{
  short int lat;
  short int lon;
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
int   STmaxSize(ST st);
void  STinsert(ST st, short int lat, short int lon, int i);
int   STsearch(ST st, short int lat, short int lon);
Coord STsearchByIndex(ST st, int i);

#endif

