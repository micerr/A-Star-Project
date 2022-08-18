#ifndef ST_H
#define ST_H

typedef struct coord{
  short int c1;
  short int c2;
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
int   STmaxSize(ST st);
void  STinsert(ST st, short int coord1, short int coord2, int i);
int   STsearch(ST st, short int coord1, short int coord2);
Coord STsearchByIndex(ST st, int i);

#endif

