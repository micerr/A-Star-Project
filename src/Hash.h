#ifndef HASH_H
#define HASH_H

#include "Graph.h"

typedef struct hash_s *Hash;

Hash HASHinit(Graph G, int numTh, int (*hfunc)(Hash h, int v));
void HASHfree(Hash h);
int hash(Hash h, int v);
int randomHashing(Hash h, int v);
int multiplicativeHashing(Hash h, int v);
int zobristHashing(Hash h, int v);
int abstractStateZobristHashing(Hash h, int v);
int abstractFeatureZobristHashing(Hash h, int v);
int HASHgetByteSize(Hash h);

#endif