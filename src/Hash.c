#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "Hash.h"
#include "Graph.h"
#include "utility/Timer.h"

#define GOLD_RATIO (sqrt(5)-1)/2  
#define C 10000.0

struct hash_s{
    int **T; // (t, 2^r), thus r should not be too large
    int (*hfunc)(Hash h, int v);
    int numTh;
    int p; // it's the size of the input for the Zobrist
    int r; // it's the block size (power of two number)
    int t; // it's the number of blocks per string 
    Graph G;
};


static int getState(Graph G, int v);
static int mulHash(int k, int m);
static int abstraction(int s);

Hash HASHinit(Graph G, int numTh, int (*hfunc)(Hash h, int v)){
    Hash temp = malloc(sizeof(*temp));
    if(temp == NULL)
        exit(1);

    // elementary parameters
    int p = sizeof(int)*8;// 32 bits, it's the size of the input for the Zobrist
    int r = 8; //bit, it's the block size (power of two number)
    int t = p/r; // 4 blocks, it's the number of blocks per string 

    if(hfunc == zobristHashing 
        || hfunc == abstractFeatureZobristHashing
        || hfunc == abstractStateZobristHashing){
        // elementary parameters
        temp->p = p;
        temp->r = r;
        temp->t = t;

        // init T
        temp->T = malloc(t*sizeof(int*));
        if(temp->T == NULL){
            perror("Error allocating T of hash funct: ");
            exit(1);
        }
        
        srand(time(NULL));
        for(int i=0; i<t; i++){
            temp->T[i]= malloc((1<<r)*sizeof(int));
            if(temp->T[i] == NULL){
                perror("Error allocating T[i] of hash funct: ");
                exit(1);
            }
            for(int j=0; j<(1<<r); j++)
                if(hfunc == abstractFeatureZobristHashing)
                    temp->T[i][j] = abstraction(rand());
                else
                    temp->T[i][j] = rand();
                
        }

    }
    temp->G = G;
    temp->numTh = numTh;
    temp->hfunc = hfunc;

    return temp;
}

void HASHfree(Hash h){
    if(h->hfunc == zobristHashing || h->hfunc == abstractStateZobristHashing || h->hfunc == abstractFeatureZobristHashing){
        for(int i=0; i<h->t; i++)
            free(h->T[i]);
        free(h->T);
    }
    free(h);
}

int hash(Hash h, int v){
    return h->hfunc(h, v);
}


// random
int randomHashing(Hash h, int v){
    return v%h->numTh;
}

/*
    Random multiplicative
    K := key
    M := module
*/
int multiplicativeHashing(Hash h, int v){
    unsigned int x = getState(h->G, v);
    return mulHash(x, h->numTh);
}

int zobristHashing(Hash h, int v){
    unsigned int x = getState(h->G, v);

    unsigned int res = 0; // sizeof(res) = 32 bit
    for(int i=0; i<h->t; i++){
        res ^=(h->T)[i][(unsigned char)(x >> (h->r * i))];
    }

    return mulHash(res, h->numTh);
}

int abstractFeatureZobristHashing(Hash h, int v){
    // change the initialization
    return zobristHashing(h, v);
}

int abstractStateZobristHashing(Hash h, int v){
    unsigned int x = getState(h->G, v);
    x = abstraction(x);

    unsigned int res = 0; // sizeof(res) = 32 bit
    for(int i=0; i<h->t; i++){
        res ^=(h->T)[i][(unsigned char)(x >> (h->r * i))];
    }

    return mulHash(res, h->numTh);
}

static int getState(Graph G, int v){
    Coord state = STsearchByIndex(G->coords, v);
    short int lat=(double)state->c1/C, lon=(double)state->c2/C;
    return (lat<<16) + lon; // input string
}

static int mulHash(int k, int m){
    double K = GOLD_RATIO*k;
    return (int) floor((double)m*(K - floor(K)));
}

static int abstraction(int s){
    short int lon = (short int) s;
    short int lat = (short int)(s>>16);

    int a = (double)lat/100.0+90, b = (double)lon/100.0+180;
    int c = a*8000, d = b+8000;
    short int first=c/100, second=d/100;

    return (first<<16) + second;    
}

int HASHgetByteSize(Hash h){
    int size =0;
    if(h->hfunc == zobristHashing 
        || h->hfunc == abstractFeatureZobristHashing
        || h->hfunc == abstractStateZobristHashing){
            size += sizeof(int*) * h->t;
            size += sizeof(int) * (1<<h->r)* h->t;
        }
    return size;
}