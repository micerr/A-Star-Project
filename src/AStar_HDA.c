#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <semaphore.h>

#include "AStar.h"
#include "Queue.h"
#include "PQ.h"
#include "Hash.h"
#include "./utility/Timer.h"


typedef struct{
    pthread_t tid;
    int numTH;
    int start, end;
    int isMaster;
    int *stop, *nStops;
    int *nMsgSnt;
    int *nMsgRcv;
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    Queue **queueMat_S2S;
    sem_t **semS;
    sem_t *semM;
    pthread_mutex_t *meStop;
    Hash hash;
    #if defined TIME || defined ANALYTICS
        Timer timer;
    #endif
} masterArg_t;

typedef struct{
    pthread_t tid;
    int numTH;
    int id;
    int start, end;
    int isMaster;
    int *hScores;
    int *bCost;
    int *path;
    int *nMsgSnt;
    int *nMsgRcv;
    int *stop, *nStops;
    int pqSize;
    Queue S2M, M2S;
    Queue **S2S;
    pthread_mutex_t *meCost;
    pthread_mutex_t *meStop;
    sem_t *semToDo;
    sem_t **semS;
    sem_t *semM;
    Graph G;
    Hash hash;
    #if defined TIME || defined ANALYTICS
        Timer timer;
        int *nExtractions, *maxNodeAssigned, *avgNodeAssigned, *selfAnalyzed;
        pthread_mutex_t *meStats;
    #endif 
} slaveArg_t;

static int search_type;

static Analytics ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int isMaster, int type, int (*hfunc)(Hash h, int v));
static void *masterTH(void *par);
static void *slaveTH(void *par);
static void analyzeNode(slaveArg_t *arg, PQ openSet, int *closedSet, HItem message);

// Wrapper for HDA* with deliver Master
Analytics ASTARhdaMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type, int (*hfunc)(Hash h, int v)){
    return ASTARhda(G, start, end, numTH, h, 1, search_type, hfunc);
}

// Wrapper for HDA* withOUT deliver Master
Analytics ASTARhdaNoMaster(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int search_type, int (*hfunc)(Hash h, int v)){
    return ASTARhda(G, start, end, numTH, h, 0, search_type, hfunc);
}

static Analytics ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord), int isMaster, int type, int (*hfunc)(Hash h, int v)){
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    Queue **queueMat_S2S;
    pthread_mutex_t *meCost, *meStop;
    masterArg_t masterArg;
    slaveArg_t *slaveArgArr;
    Coord coord, dest_coord;
    sem_t **semS;
    sem_t *semM;
    Hash hash;
    int i, j, *hScores, *path, bCost = INT_MAX, stop = 0, nStops=0;
    int *nMsgSnt, *nMsgRcv;

    search_type = type;

    //create the array containing all precomputed Hscores
    hScores = malloc(G->V * sizeof(int));
    if(hScores == NULL){
        perror("Error allocating hScores: ");
        exit(1);
    }
    dest_coord = STsearchByIndex(G->coords, end);
    for(int i=0; i<G->V; i++){
        coord = STsearchByIndex(G->coords, i);
        hScores[i] = h(coord, dest_coord);
    }

    //init the path array
    path = malloc(G->V * sizeof(int));
    if(path == NULL){
        perror("Error allocating path array: ");
        exit(1);
    }
    for(int i=0; i<G->V; i++)
        path[i] = -1;
    
    if(isMaster){

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

    }else{

        // allocate the matrix of Slave-to-Salve queues
        queueMat_S2S = malloc(numTH * sizeof(Queue*));
        if(queueMat_S2S == NULL){
            perror("Error allocating queueMat_S2S: ");
            exit(1);
        }
        for(i=0; i<numTH; i++){
            queueMat_S2S[i] = malloc(numTH * sizeof(Queue));
            if(queueMat_S2S[i] == NULL){
                perror("Error allocating queueMat_S2S: ");
                exit(1);
            }
            for(j=0; j<numTH; j++)
                queueMat_S2S[i][j] = QUEUEinit();
        }
    }


    meStop = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(meStop, NULL);

    meCost = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(meCost, NULL);

    //init the array of slaves' semaphores
    semS = malloc(numTH * sizeof(sem_t *));
    if(semS == NULL){
        perror("Error trying to allocate semaphores for slaves: ");
        exit(1);
    }
    for(i=0; i<numTH; i++){
        semS[i] = malloc(sizeof(sem_t));
        sem_init(semS[i], 0, 0);
    }

    //init master's semaphore
    semM = malloc(sizeof(sem_t));
    if(semM == NULL){
        perror("Error trying to allocate master's semaphore: ");
        exit(1);
    }
    sem_init(semM, 0, 0);

    // init the array of messages
    nMsgRcv = malloc(numTH * sizeof(int));
    nMsgSnt = malloc(numTH * sizeof(int));
    for(i = 0; i<numTH; i++)
        nMsgRcv[i] = nMsgSnt[i] = 0;

    // init the hash function
    hash = HASHinit(G, numTH, hfunc);

    #if defined TIME || defined ANALYTICS
        Timer timer;
        timer = TIMERinit(numTH);
        int nExtractions = 0, maxNodeAssigned=0, avgNodeAssigned=0, selfAnalyzed=0;
        pthread_mutex_t meStats = PTHREAD_MUTEX_INITIALIZER;
    #endif

    //create the arg-structure of the master
    if(isMaster){
        masterArg.queueArr_M2S = queueArr_M2S;
        masterArg.queueArr_S2M = queueArr_S2M;
    }else{
        masterArg.queueMat_S2S = queueMat_S2S;
    }
    masterArg.numTH = numTH;
    masterArg.start = start;
    masterArg.end = end;
    masterArg.stop = &stop;
    masterArg.nStops = &nStops;
    masterArg.nMsgRcv = nMsgRcv;
    masterArg.nMsgSnt = nMsgSnt;
    masterArg.semM = semM;
    masterArg.semS = semS;
    masterArg.meStop = meStop;
    masterArg.isMaster = isMaster;
    masterArg.hash = hash;
    #if defined TIME || defined ANALYTICS
        masterArg.timer = timer;
    #endif

    //create the array of slaves' arg-struct
    slaveArgArr = malloc(numTH * sizeof(slaveArg_t));
    for(i=0; i<numTH; i++){
        if(isMaster){
            slaveArgArr[i].M2S = queueArr_M2S[i];
            slaveArgArr[i].S2M = queueArr_S2M[i];
        }else{
            slaveArgArr[i].S2S = queueMat_S2S;
        }
        slaveArgArr[i].numTH = numTH;
        slaveArgArr[i].hScores = hScores;
        slaveArgArr[i].path = path;
        slaveArgArr[i].bCost = &bCost;
        slaveArgArr[i].id = i;
        slaveArgArr[i].G = G;
        slaveArgArr[i].meCost = meCost;
        slaveArgArr[i].meStop = meStop;
        slaveArgArr[i].stop = &stop;
        slaveArgArr[i].nStops = &nStops;
        slaveArgArr[i].start = start;
        slaveArgArr[i].end = end;
        slaveArgArr[i].nMsgRcv = &(nMsgRcv[i]);
        slaveArgArr[i].nMsgSnt = &(nMsgSnt[i]);
        slaveArgArr[i].semM = semM;
        slaveArgArr[i].semToDo = semS[i];
        slaveArgArr[i].semS = semS;
        slaveArgArr[i].isMaster = isMaster;
        slaveArgArr[i].hash = hash;
        #if defined TIME || defined ANALYTICS
            slaveArgArr[i].timer = timer;
            slaveArgArr[i].maxNodeAssigned = &maxNodeAssigned;
            slaveArgArr[i].avgNodeAssigned = &avgNodeAssigned;
            slaveArgArr[i].nExtractions = &nExtractions;
            slaveArgArr[i].meStats = &meStats;
            slaveArgArr[i].selfAnalyzed = &selfAnalyzed;
        #endif
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

    Analytics stats = NULL;
    #ifdef ANALYTICS
        int size = 0;
        size += G->V * sizeof(int);     // hscores
        size += G->V * sizeof(int);     // path
        size += numTH*2 * sizeof(int);   // messages
        size += sizeof(void*)*(12+numTH);       // pointers
        size += sizeof(pthread_mutex_t)*2 + sizeof(sem_t)*(1+numTH);
        size += HASHgetByteSize(hash);
        if(isMaster){
            size += 2*numTH * sizeof(Queue);
            size += 2*numTH * (sizeof(void *)*4 + sizeof(pthread_mutex_t)*2 );
            size += avgNodeAssigned * sizeof(HItem*); // avg right now is the sum and NOT the avg
        }else{
            size += numTH*numTH * sizeof(Queue);
            size += numTH*numTH * (sizeof(void *)*4 + sizeof(pthread_mutex_t)*2 );
            size += avgNodeAssigned * sizeof(HItem*); // avg right now is the sum and NOT the avg
        }
        size += sizeof(masterArg_t) + sizeof(slaveArg_t) * numTH;
        size += numTH*G->V * sizeof(int); // closedSet
        for(i=0; i<numTH; i++)
            size += slaveArgArr[i].pqSize;

        int S = 0;
        for(i=0; i<numTH; i++)
            S += nMsgSnt[i];
        stats = ANALYTICSsave(G, start, end, path, bCost, nExtractions, maxNodeAssigned, avgNodeAssigned/numTH, 1-(float)selfAnalyzed/S, TIMERgetElapsed(timer), size);
    #endif

    #ifndef ANALYTICS
    if(path[end]==-1)
        printf("No path from %d to %d has been found.\n", start, end);
    else{
        printf("Path from %d to %d has been found with cost %d\n", start, end, bCost);
        int hops=0;
        for(int i=end; i!=start; i=path[i]){
            printf("%d <- ", i);
            hops++;
        }
        printf("%d\nHops: %d\n", start, hops);
    }
    #endif

    #if defined TIME || defined ANALYTICS
        TIMERfree(timer);
    #endif

    // free the hash function
    HASHfree(hash);

    // free the array of messages
    free(nMsgRcv); free(nMsgSnt);

    //free the array of slaves' semaphores
    for(i=0; i<numTH; i++){
        sem_destroy(semS[i]);
        free(semS[i]);
    }
    free(semS);

    //free master's semaphore
    sem_destroy(semM);
    free(semM);

    if(isMaster){

        //free the array of Slave-to-Master queues
        for(i=0; i<numTH; i++)
            QUEUEfree(queueArr_S2M[i]);
        free(queueArr_S2M);

        //free the array of Master-to-Slave queues
        for(i=0; i<numTH; i++)
            QUEUEfree(queueArr_M2S[i]);
        free(queueArr_M2S);

    }else{

        //free the matrxi of Slave-to-Slave queues
        for(i=0; i<numTH; i++){
            for(j=0; j<numTH; j++)
                QUEUEfree(queueMat_S2S[i][j]);
            free(queueMat_S2S[i]);
        }
        free(queueMat_S2S);

    }
    //free meStop
    pthread_mutex_destroy(meStop);
    free(meStop);

    //free meCost mutex
    pthread_mutex_destroy(meCost);
    free(meCost);

    //free the array of slaves' arg-struct
    free(slaveArgArr);

    free(hScores);
    free(path);

    return stats;
}

static void *masterTH(void *par){
    int i, owner;
    int R=-1, S, pR; // termination detection Mattern's Method
    HItem message;
    masterArg_t *arg = (masterArg_t *)par;

    #if defined TIME || defined ANALYTICS
        TIMERstart(arg->timer);
    #endif  

    //compute the owner of the starting thread
    owner = hash(arg->hash, arg->start);
    message = HITEMinit(arg->start, 0, arg->start, owner, NULL);

    //insert the node in the owner's queue
    if(arg->isMaster)
        QUEUEtailInsert(arg->queueArr_M2S[owner], message);
    else
        QUEUEtailInsert(arg->queueMat_S2S[owner][owner], message);

    sem_post(arg->semS[owner]); // If the slave starts before the master, we need to signal to it the insertion in the queue

    while(1){
        // special trick used to stop the master if the is no elements in the queues
        sem_wait(arg->semM); // wait for some element to be in the queue
        sem_post(arg->semM); // Reset to the previous valute, becuse in the loop we count, how many elements we read.

        for(i=0; i<arg->numTH && arg->isMaster; i++){
                while(!QUEUEisEmpty(arg->queueArr_S2M[i])){
                    message = QUEUEheadExtract(arg->queueArr_S2M[i]);

                    owner = message->owner;
                    sem_wait(arg->semM);  // decrement the number of elementes in the queues, must be non-blocking, since his construction
                    #ifdef DEBUG
                        printf("M: send from %d to %d message(n=%d, n'=%d)\n", i, message->owner, message->father, message->index);
                    #endif
                    QUEUEtailInsert(arg->queueArr_M2S[owner], message);
                    sem_post(arg->semS[owner]);

                }
        }

        // termination detection Mattern's Method
        pR = R; // save the previuos
        R = S = 0;
        for(i=0; i<arg->numTH; i++){
            R += arg->nMsgRcv[i];
            S += arg->nMsgSnt[i];
        }
        /* 
            pR == S := Mattern's condition for distributed termination detection
            S != 0  := if there are no message sent, the algorithm is not started yet
            pR = R  := the number of Received message must not be changed
            nStops = numTH := all the slave must sleep
            pR-1 because the first message is sent by the Master, and the master is not in the game
        */
        #ifdef DEBUG
            printf("R: %d, R': %d, S': %d\n", pR-1, R-1, S);
        #endif
        pthread_mutex_lock(arg->meStop);
            if(pR == R && pR-1 == S && S != 0 && *(arg->nStops) == arg->numTH){
                *(arg->stop) = 1;
                for(i=0; i<arg->numTH; i++)
                    sem_post(arg->semS[i]);
                pthread_mutex_unlock(arg->meStop);
                pthread_exit(NULL);
            }
        pthread_mutex_unlock(arg->meStop);

    }
}

static void *slaveTH(void *par){
    slaveArg_t *arg = (slaveArg_t *)par;

    PQ openSet;
    int *closedSet;
    int nRcv=0, nSnt=0;
    int i, owner;
    HItem message;
    Item extrNode;
    ptr_node t;
    int gScore, fScore, newGscore;
    #ifdef ANALYTICS
        int nExtractions=0, selfAnalyzed=0;
    #endif

    //init the openSet PQ
    openSet = PQinit(arg->G->V, search_type);
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

    while(1){

        // update the values of messages sent and received
        *(arg->nMsgSnt) += nSnt;
        *(arg->nMsgRcv) += nRcv;
        nRcv = nSnt = 0;  // Zero the counter of received and sent message of this wave

        // special trick used to stop the slave if the is no things to do
        if(sem_trywait(arg->semToDo) != 0){
            // enter here only when we have to wait, that it means that we don't have any work to do => (We have to ask to the Master if the algorithm is ended)
            pthread_mutex_lock(arg->meStop);
                *(arg->nStops)+=1;
                if(*(arg->nStops) == arg->numTH)
                    sem_post(arg->semM);
            pthread_mutex_unlock(arg->meStop);

            sem_wait(arg->semToDo);

            pthread_mutex_lock(arg->meStop);
                if(*(arg->nStops) == arg->numTH)
                    sem_wait(arg->semM);
                *(arg->nStops)-=1;
            pthread_mutex_unlock(arg->meStop);
        }
        sem_post(arg->semToDo); // Reset to the previous valute, because in the loops we count the number of elements in the queue through the semaphores.
        
        if(*(arg->stop)){
            #ifdef ANALYTICS 
                TIMERstop(arg->timer);
            #endif
            #ifdef TIME
                TIMERstopEprint(arg->timer);
            #endif
            #if defined TIME || defined ANALYTICS
                int expandedNodes = 0;
                for(i=0; i<arg->G->V; i++)
                    if(closedSet[i] > 0)
                        expandedNodes++;
            #endif
            #ifdef TIME
                printf("%d: Expanded nodes: %d\n", arg->id, expandedNodes);
            #endif
            #ifdef ANALYTICS
                pthread_mutex_lock(arg->meStats);
                if(expandedNodes > *(arg->maxNodeAssigned))
                    *(arg->maxNodeAssigned) = expandedNodes;
                *(arg->avgNodeAssigned) += expandedNodes;
                *arg->nExtractions += nExtractions;
                *arg->selfAnalyzed += selfAnalyzed;
                pthread_mutex_unlock(arg->meStats);
                arg->pqSize = PQgetByteSize(openSet);
            #endif
            //destroy openSet PQ
            PQfree(openSet);
            //destroy closedSet
            free(closedSet);
            pthread_exit(NULL);
        }

        while(!QUEUEsAreEmpty(
            arg->isMaster ? (&(arg->M2S)) : arg->S2S[arg->id],
            arg->isMaster ? 1 : arg->numTH, 
            &i
        )){
            message = QUEUEsHeadExtract(
                arg->isMaster ? &(arg->M2S) : arg->S2S[arg->id],
                arg->isMaster ? 1 : arg->numTH,
                i
            );

            // decrement the number of thing that we have to do
            sem_wait(arg->semToDo); // must be non-blocking, since his construction
            nRcv++; // increment the received messages

            analyzeNode(arg, openSet, closedSet, message);
            
            free(message); // a message is never freedzed
        }

        if(PQempty(openSet))
            continue;
        
        extrNode = PQextractMin(openSet);
        closedSet[extrNode.index] = extrNode.priority;
        // decrement the number of thing that we have to do
        sem_wait(arg->semToDo); // must be non-blocking, since his construction

        #ifdef ANALYTICS
            nExtractions++;
        #endif

        #ifdef DEBUG
            printf("%d: Extracted %d\n", arg->id, extrNode.index);
        #endif

        pthread_mutex_lock(arg->meCost);
        if(extrNode.priority >= *(arg->bCost)){
            pthread_mutex_unlock(arg->meCost);
            continue;
        }
        pthread_mutex_unlock(arg->meCost);

        if(extrNode.index == arg->end){
            pthread_mutex_lock(arg->meCost);
            if(extrNode.priority < *(arg->bCost)){     
                // save the cost
                *(arg->bCost) = extrNode.priority;
                #ifdef DEBUG
                    printf("%d: best %d\n", arg->id, *(arg->bCost));
                #endif
            }
            pthread_mutex_unlock(arg->meCost);    
            continue;
        }

        fScore = extrNode.priority;
        gScore = fScore - arg->hScores[extrNode.index];

        for(t=arg->G->ladj[extrNode.index]; t!=arg->G->z; t=t->next){
            newGscore = gScore + t->wt;
            #ifdef DEBUG
                printf("%d: expanded node %d->%d\n", arg->id, extrNode.index, t->v);
            #endif

            // same check is done before, but this will avoid the insertion in the queue
            // no mutex, we don't want slow down the algorithm
            // the worst case is that the the node is put in the queue, but isn't a problem
            // beacuse the same check is done after
            if(newGscore + arg->hScores[t->v] >= *(arg->bCost))
                continue;
            
            // pruning
            if(arg->path[extrNode.index] == t->v)
                continue;

            owner = hash(arg->hash, t->v);
            message = HITEMinit(t->v, newGscore, extrNode.index, owner, NULL);

            if(owner == arg->id){
                analyzeNode(arg, openSet, closedSet, message);
                free(message);
                nRcv ++; // Because we have aready done the work
                #ifdef ANALYTICS
                    selfAnalyzed++;
                #endif
            }else if(arg->isMaster){
                QUEUEtailInsert(arg->S2M, message);
                sem_post(arg->semM); // tells the master that there is a message in the queue
            }else{
                QUEUEtailInsert(arg->S2S[owner][arg->id], message);
                sem_post(arg->semS[owner]); // notify the owner that there is a message
                #ifdef DEBUG
                    printf("send from %d to %d message(n=%d, n'=%d)\n", arg->id, message->owner, message->father, message->index);
                #endif
            }
            nSnt++; // increment the number of messages sent
        }
    }


    //destroy openSet PQ
    PQfree(openSet);

    //destroy closedSet
    free(closedSet);

    pthread_exit(NULL);
}

static void analyzeNode(slaveArg_t *arg, PQ openSet, int *closedSet, HItem message){
    int newFscore, newGscore;
    int fScore, gScore;

    #ifdef DEBUG
        printf("%d: analyzing %d\n", arg->id, message->index);
    #endif

    newGscore = message->priority;

    //if it belongs to the closed set
    if(closedSet[message->index] >= 0){
        gScore = closedSet[message->index] - arg->hScores[message->index];
        
        //if new gScore is lower than the previous one
        if(newGscore < gScore){
            newFscore = newGscore + arg->hScores[message->index];
            //remove it from the closed set
            closedSet[message->index] = -1;
            //add it to the openSet1
            PQinsert(openSet, message->index, newFscore);
            // increment the number of thing that we have to do
            sem_post(arg->semToDo); 
        }
        else{
            return;
        }
            
    }
    //if it doesn't belong to the closed set
    else{
        //if it doesn't belongs to the open set
        if(PQsearch(openSet, message->index, &fScore) < 0){
            newFscore = newGscore + arg->hScores[message->index];
            PQinsert(openSet, message->index, newFscore);
            // increment the number of thing that we have to do
            sem_post(arg->semToDo); 
        }
        else{
            gScore = fScore - arg->hScores[message->index];
            newFscore = newGscore + arg->hScores[message->index];
            //if it belongs to the closed set with an higher gScore, change is priority
            if(newGscore < gScore)
                PQchange(openSet, message->index, newFscore);
            else{
                return;
            }
                
        }
    }
    arg->path[message->index] = message->father;

    return;
}
