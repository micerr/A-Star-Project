#ifndef ST_H
#define ST_H

typedef struct coord{
<<<<<<< HEAD
  int c1;
  int c2;
=======
  short int lat;
  short int lon;
>>>>>>> ebd5fdf55a1bd3617d418f8ad3c8b1d7617df58b
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
int   STmaxSize(ST st);
<<<<<<< HEAD
void  STinsert(ST st, int coord1, int coord2, int i);
int   STsearch(ST st, int coord1, int coord2);
=======
void  STinsert(ST st, short int lat, short int lon, int i);
int   STsearch(ST st, short int lat, short int lon);
>>>>>>> ebd5fdf55a1bd3617d418f8ad3c8b1d7617df58b
Coord STsearchByIndex(ST st, int i);

#endif

