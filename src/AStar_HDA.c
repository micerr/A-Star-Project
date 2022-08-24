#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

#include "AStar.h"
#include "Queue.h"
#include "PQ.h"

typedef struct{
    pthread_t tid;
    int numTH;
    int start, end;
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
} masterArg_t;

typedef struct{
    pthread_t tid;
    int numTH;
    int id;
    int start, end;
    int *hScores;
    int *bCost, *n;
    int *path;
    Queue S2M;
    Queue M2S;
    Graph G;
} slaveArg_t;

static void *masterTH(void *par);
static void *slaveTH(void *par);
static int multiplicativeHashing(int s);
static int terminateDetection();

void ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord)){
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    masterArg_t masterArg;
    slaveArg_t *slaveArgArr;
    Coord coord, dest_coord;
    int *hScores, *path, bCost = INT_MAX;

    //create the array containing all precomputed Hscores
    hScores = malloc(G->V * sizeof(int));
    if(hScores == NULL){
        perror("Error allocating hScores: ");
        exit(1);
    }
    dest_coord = STsearchByIndex(G->coords, end);
    for(int i=0; i<G->V; i++){
        coord = STsearchByIndex(G->coords, i);
        hScores = h(coord, dest_coord);
    }

    //init the path array
    path = malloc(G->V * sizeof(int));
    if(path == NULL){
        perror("Error allocating path array: ");
        exit(1);
    }
    for(int i=0; i<G->V; i++)
        path[i] = -1;



    // allocate the array of Slave-to-Master queues
    queueArr_S2M = malloc(numTH * sizeof(Queue));
    if(queueArr_S2M == NULL){
        perror("Error allocating queueArr_S2M: ");
        exit(1);
    }
    for(int i=0; i<numTH; i++)
        queueArr_S2M[i] = QUEUEinit();
    
    // allocate the array of Master-to-Slave queues
    queueArr_M2S = malloc(numTH * sizeof(Queue));
    if(queueArr_M2S == NULL){
        perror("Error allocating queueArr_M2S: ");
        exit(1);
    }
    for(int i=0; i<numTH; i++)
        queueArr_M2S[i] = QUEUEinit();

    //create the arg-structure of the master
    masterArg.numTH = numTH;
    masterArg.queueArr_M2S = queueArr_M2S;
    masterArg.queueArr_S2M = queueArr_S2M;

    //create the array of slaves' arg-struct
    slaveArgArr = malloc(numTH * sizeof(slaveArg_t));
    for(int i=0; i<numTH; i++){
        slaveArgArr[i].numTH = numTH;
        slaveArgArr[i].M2S = queueArr_M2S[i];
        slaveArgArr[i].S2M = queueArr_S2M[i];
        slaveArgArr[i].hScores = hScores;
        slaveArgArr[i].path = path;
        slaveArgArr[i].bCost = &bCost;
        slaveArgArr[i].G = G;
    }

    //start the master
    pthread_create(&masterArg.tid, NULL, masterTH, (void *)&masterArg);

    //start all slaves
    for(int i=0; i<numTH; i++){
        slaveArgArr[i].id = i;
        pthread_create(&slaveArgArr[i].tid, NULL, slaveTH, (void *)&slaveArgArr[i]);
    }

    //wait termination of all slaves
    for(int i=0; i<numTH; i++)
        pthread_join(slaveArgArr[i].tid, NULL);

    //wait termination of the master
    pthread_join(masterArg.tid, NULL);






    //free the array of Slave-to-Master queues
    for(int i=0; i<numTH; i++)
        QUEUEfree(queueArr_S2M[i]);
    free(queueArr_S2M);

    //free the array of Master-to-Slave queues
    for(int i=0; i<numTH; i++)
        QUEUEfree(queueArr_M2S[i]);
    free(queueArr_M2S);

    //free the array of slaves' arg-struct
    free(slaveArgArr);

    free(hScores);
    free(path);

}

/* TODO: condizione di terminazione. 
        Utilizzando il master forse diventa più semplice. Esso controlla
        tutte le code: se tutte le code S2M sono vuote, allora vuol dire che 
        gli slave non stanno più esplorando nodi, e se le code M2S sono vuote
        vuol dire che tutti i master hanno estratto i nodi.
        Causa problemi?
*/
static void *masterTH(void *par){
    int i, owner;
    HItem message;
    masterArg_t *arg = (masterArg_t *)par;

    //compute the owner of the starting thread
    owner = multiplicativeHashing(arg->start);

    //insert the node in the owner's queue
    QUEUEtailInsert(arg->queueArr_M2S[owner], HITEMinit(arg->start, 0, -1, NULL));

    while(1){
        for(i=0; i<arg->numTH; i++)
        // TODO: vanno inseriti i lock (eventualmente trylock)
            while(!QUEUEisEmpty(arg->queueArr_S2M[i])){
                message = QUEUEheadExtract(arg->queueArr_S2M[i]);
                owner = multiplicativeHashing(message->index);
                QUEUEtailInsert(arg->queueArr_M2S[owner], message);
            }
    }




    pthread_exit(NULL);
}

static void *slaveTH(void *par){
    slaveArg_t *arg = (slaveArg_t *)par;

    PQ openSet;
    int *closedSet;

    HItem message;
    int gScore, fScore, newGscore, newFscore;

    //init the openSet PQ
    openSet = PQinit(64);
    if(openSet == NULL){
        printf("Error initializing openSet (PQ) in thread%d: ", arg->id);
        exit(1);
    }

    //init closedSet
    closedSet = malloc(arg->G->V * sizeof(int));
     if(closedSet == NULL){
        printf("Error initializing closedSet (PQ) in thread%d: ", arg->id);
        exit(1);
    }
    for(int i=0; i<arg->G->V; i++)
        closedSet[i] = -1;

    while(terminateDetection){
        // TODO: potremmo aggiungere una conditional variable: se sia M2S che openSet sono vuote
        //      (che è la condizone di terminazione parziale del singolo thread), il thread
        //      viene messo a dormire e viene svegliato quando il master aggiunge un messaggio
        //      nella sua coda, oppure quando il master si accorge che tutti i thread sono in
        //      in quella situazione quindi li sveglia tutti per farli terminare (quindi dopo il
        //      risveglio il singolo thread dovrebbe controllare una condizione per capire se deve 
        //      terminare).

        while(!QUEUEisEmpty(arg->M2S)){
            //extract a message from the queue
            message = QUEUEheadExtract(arg->M2S);
            newGscore = message->priority;

            //if it belongs to the closed set
            if(closedSet[message->index] >= 0){
                gScore = closedSet[message->index] - arg->hScores[message->index];
                
                //if new gScore is lower than the previous one
                if(newGscore < gScore){
                    newFscore = newGscore + arg->hScores[message->index];
                    //remove it from the closed set
                    closedSet[message->index] = -1;
                    //add it to the openSet
                    PQinsert(openSet, message->index, newFscore);
                }
                else
                    continue;
            }
            //if it doesn't belong to the closed set
            else{
                //if it doesn't belongs to the open set
                if(PQsearch(openSet, message->index, &fScore) < 0){
                    newFscore = newGscore + arg->hScores[message->index];
                    PQinsert(openSet, message->index, newFscore);
                }
                else{
                    gScore = fScore - arg->hScores[message->index];
                    newFscore = newGscore + arg->hScores[message->index];
                    //if it belongs to the closed set with an higher gScore, change is priority
                    if(newGscore < gScore)
                        PQchange(openSet, message->index, newFscore);
                    else 
                        continue;
                }
            }
            arg->path[message->index] = message->father;
        }
    }

    



    //destroy openSet PQ
    PQfree(openSet);

    //destroy closedSet
    free(closedSet);

    pthread_exit(NULL);
}

static int multiplicativeHashing(int s){
    return 1;
}

static int terminateDetection(){
    return 1;
}