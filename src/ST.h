#ifndef ST_H
#define ST_H

typedef struct coord{
  short c1;
  short c2;
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
void  STinsert(ST st, short coord1, short coord2, int i);
int   STsearch(ST st, short coord1, short coord2);
Coord STsearchByIndex(ST st, int i);

#endif

