#ifndef ST_H
#define ST_H

typedef struct coord{
  float c1;
  float c2;
} *Coord;

typedef struct symboltable *ST;

ST    STinit(int maxN);
void  STfree(ST st);
int   STsize(ST st);
int   STmaxSize(ST st);
void  STinsert(ST st, float coord1, float coord2, int i);
Coord STsearchByIndex(ST st, int i);

#endif

