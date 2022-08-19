#ifndef ST_H
#define ST_H

typedef struct coord{
  int c1;
  int c2;
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
int   STmaxSize(ST st);
void  STinsert(ST st, int coord1, int coord2, int i);
int   STsearch(ST st, int coord1, int coord2);
Coord STsearchByIndex(ST st, int i);

#endif

