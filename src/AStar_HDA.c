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
    pthread_mutex_t **meS2M;
    pthread_mutex_t **meM2S;
} masterArg_t;

typedef struct{
    pthread_t tid;
    int numTH;
    int id;
    int start, end;
    int *hScores;
    int *bCost, *n;
    int *path;
    Queue S2M, M2S;
    pthread_mutex_t *meS2M, *meM2S;
    Graph G;
} slaveArg_t;

static void *masterTH(void *par);
static void *slaveTH(void *par);
static int multiplicativeHashing(int s);
static int terminateDetection();

void ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord)){
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    pthread_mutex_t **meS2M;
    pthread_mutex_t **meM2S;
    
    masterArg_t masterArg;
    slaveArg_t *slaveArgArr;
    Coord coord, dest_coord;
    int i, *hScores, *path, bCost = INT_MAX;

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
    for(i=0; i<numTH; i++)
        queueArr_S2M[i] = QUEUEinit();
    
    // allocate the array of Master-to-Slave queues
    queueArr_M2S = malloc(numTH * sizeof(Queue));
    if(queueArr_M2S == NULL){
        perror("Error allocating queueArr_M2S: ");
        exit(1);
    }
    for(i=0; i<numTH; i++)
        queueArr_M2S[i] = QUEUEinit();

    //allocate and init the array of M2S mutexes
    meM2S = malloc(numTH * sizeof(pthread_mutex_t*));
    for(i=0; i<numTH; i++){
        meM2S[i] = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(meM2S[i], NULL);
    }

    //allocate and init the array of M2S mutexes
    meS2M = malloc(numTH * sizeof(pthread_mutex_t*));
    for(i=0; i<numTH; i++){
        meS2M[i] = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(meS2M[i], NULL);
    }

    //create the arg-structure of the master
    masterArg.numTH = numTH;
    masterArg.start = start;
    masterArg.end = end;
    masterArg.queueArr_M2S = queueArr_M2S;
    masterArg.queueArr_S2M = queueArr_S2M;
    masterArg.meM2S = meM2S;
    masterArg.meS2M = meS2M;

    //create the array of slaves' arg-struct
    slaveArgArr = malloc(numTH * sizeof(slaveArg_t));
    for(i=0; i<numTH; i++){
        slaveArgArr[i].numTH = numTH;
        slaveArgArr[i].M2S = queueArr_M2S[i];
        slaveArgArr[i].S2M = queueArr_S2M[i];
        slaveArgArr[i].hScores = hScores;
        slaveArgArr[i].path = path;
        slaveArgArr[i].bCost = &bCost;
        slaveArgArr[i].id = i;
        slaveArgArr[i].G = G;
        slaveArgArr[i].meM2S = meM2S[i];
        slaveArgArr[i].meS2M = meS2M[i];
    }

    //start the master
    pthread_create(&masterArg.tid, NULL, masterTH, (void *)&masterArg);

    //start all slaves
    for(i=0; i<numTH; i++)
        pthread_create(&slaveArgArr[i].tid, NULL, slaveTH, (void *)&slaveArgArr[i]);
    

    //wait termination of all slaves
    for(i=0; i<numTH; i++)
        pthread_join(slaveArgArr[i].tid, NULL);

    //wait termination of the master
    pthread_join(masterArg.tid, NULL);






    //free the array of Slave-to-Master queues
    for(i=0; i<numTH; i++)
        QUEUEfree(queueArr_S2M[i]);
    free(queueArr_S2M);

    //free the array of Master-to-Slave queues
    for(i=0; i<numTH; i++)
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
    QUEUEtailInsert(arg->queueArr_M2S[owner], HITEMinit(arg->start, 0, arg->start, NULL));

    //TODO: 
    while(1){
        for(i=0; i<arg->numTH; i++){
            if(pthread_mutex_trylock(arg->meS2M[i]) == 0){
                while(!QUEUEisEmpty(arg->queueArr_S2M[i])){
                    message = QUEUEheadExtract(arg->queueArr_S2M[i]);
                    owner = multiplicativeHashing(message->index);
                    pthread_mutex_lock(arg->meM2S[owner]);
                    QUEUEtailInsert(arg->queueArr_M2S[owner], message);
                    pthread_mutex_unlock(arg->meM2S[owner]);
                }
            }
        }            
    }

    pthread_exit(NULL);
}

static void *slaveTH(void *par){
    slaveArg_t *arg = (slaveArg_t *)par;

    PQ openSet;
    int *closedSet;

    HItem message;
    Item extrNode;
    ptr_node t;
    int gScore, fScore, newGscore, newFscore;

    //init the openSet PQ
    openSet = PQinit(5);
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

        pthread_mutex_lock(arg->M2S);
        while(!QUEUEisEmpty(arg->M2S)){
            //extract a message from the queue
            message = QUEUEheadExtract(arg->M2S);
            pthread_mutex_unlock(arg->meM2S);
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
                else{
                    pthread_mutex_lock(arg->M2S);
                    continue;
                }
                    
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
                    else{
                        pthread_mutex_lock(arg->M2S);
                        continue;
                    }
                        
                }
            }
            arg->path[message->index] = message->father;
            pthread_mutex_lock(arg->M2S);
        }

        if(PQempty(openSet))
            continue;
        
        extrNode = PQextractMin(openSet);
        if(extrNode.priority >= arg->bCost)
            continue;

        closedSet[extrNode.index] = extrNode.priority;

        if(extrNode.index == arg->end){
            if(extrNode.priority < *(arg->bCost))          
            // save the cost
            *(arg->bCost) = extrNode.priority;
            
            continue;
        }

        fScore = extrNode.priority;
        gScore = fScore - arg->hScores[extrNode.index];

        for(t=arg->G->ladj[extrNode.index]; t!=arg->G->z; t=t->next){
            newGscore = gScore + t->wt;
            pthread_mutex_lock(arg->S2M);
            QUEUEtailInsert(arg->S2M, HITEMinit(t->v, newGscore, extrNode.index, NULL));
            pthread_mutex_unlock(arg->S2M);
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