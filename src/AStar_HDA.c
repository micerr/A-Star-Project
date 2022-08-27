#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <semaphore.h>

#include "AStar.h"
#include "Queue.h"
#include "PQ.h"
#include "./utility/BitArray.h"
#include "./utility/Timer.h"

typedef struct{
    pthread_t tid;
    int numTH;
    int start, end;
    int *stop;
    int *nMsgSnt;
    int *nMsgRcv;
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    sem_t **semS;
    sem_t *semM;
    #ifdef TIME
        Timer timer;
    #endif
} masterArg_t;

typedef struct{
    pthread_t tid;
    int numTH;
    int id;
    int start, end;
    int *hScores;
    int *bCost;
    int *path;
    int *nMsgSnt;
    int *nMsgRcv;
    int *stop;
    Queue S2M, M2S;
    pthread_mutex_t *meCost;
    sem_t *semToDo;
    sem_t *semM;
    Graph G;
    #ifdef TIME
        Timer timer;
    #endif 
} slaveArg_t;

static void *masterTH(void *par);
static void *slaveTH(void *par);
static int hashing(int s, int numTH);

void ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord)){
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    pthread_mutex_t *meCost;
    masterArg_t masterArg;
    slaveArg_t *slaveArgArr;
    Coord coord, dest_coord;
    sem_t **semS;
    sem_t *semM;
    int i, *hScores, *path, bCost = INT_MAX, stop = 0;
    int *nMsgSnt, *nMsgRcv;
    #ifdef TIME
        Timer timer;
    #endif

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

    #ifdef TIME
        timer = TIMERinit(numTH);
    #endif

    //create the arg-structure of the master
    masterArg.numTH = numTH;
    masterArg.start = start;
    masterArg.end = end;
    masterArg.queueArr_M2S = queueArr_M2S;
    masterArg.queueArr_S2M = queueArr_S2M;
    masterArg.stop = &stop;
    masterArg.nMsgRcv = nMsgRcv;
    masterArg.nMsgSnt = nMsgSnt;
    masterArg.semM = semM;
    masterArg.semS = semS;
    #ifdef TIME
        masterArg.timer = timer;
    #endif

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
        slaveArgArr[i].meCost = meCost;
        slaveArgArr[i].stop = &stop;
        slaveArgArr[i].start = start;
        slaveArgArr[i].end = end;
        slaveArgArr[i].nMsgRcv = &(nMsgRcv[i]);
        slaveArgArr[i].nMsgSnt = &(nMsgSnt[i]);
        slaveArgArr[i].semM = semM;
        slaveArgArr[i].semToDo = semS[i];
        #ifdef TIME
            slaveArgArr[i].timer = timer;
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

    #ifdef TIME
        free(timer);
    #endif

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

    //free the array of Slave-to-Master queues
    for(i=0; i<numTH; i++)
        QUEUEfree(queueArr_S2M[i]);
    free(queueArr_S2M);

    //free the array of Master-to-Slave queues
    for(i=0; i<numTH; i++)
        QUEUEfree(queueArr_M2S[i]);
    free(queueArr_M2S);

    //free meCost mutex
    pthread_mutex_destroy(meCost);
    free(meCost);

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
    int R=-1, S, pR; // termination detection Mattern's Method
    HItem message;
    masterArg_t *arg = (masterArg_t *)par;

    #ifdef TIME
        TIMERstart(arg->timer);
    #endif  

    //compute the owner of the starting thread
    owner = hashing(arg->start, arg->numTH);

    //insert the node in the owner's queue
    QUEUEtailInsert(arg->queueArr_M2S[owner], HITEMinit(arg->start, 0, arg->start, owner, NULL));
    sem_post(arg->semS[owner]); // If the slave starts before the master, we need to signal to it the insertion in the queue

    while(1){
        // special trick used to stop the master if the is no elements in the queues
        sem_wait(arg->semM); // wait for some element to be in the queue
        sem_post(arg->semM); // Reset to the previous valute, becuse in the loop we count, how many elements we read.

        for(i=0; i<arg->numTH; i++){
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
            pR-1 because the first message is sent by the Master, and the master is not in the game
        */
        #ifdef DEBUG
            printf("R: %d, R': %d, S': %d\n", pR-1, R-1, S);
        #endif
        if(pR-1 == S && S != 0){
            *(arg->stop) = 1;
            for(i=0; i<arg->numTH; i++)
                sem_post(arg->semS[i]);
            pthread_exit(NULL);
        }
    }
}

static void *slaveTH(void *par){
    slaveArg_t *arg = (slaveArg_t *)par;

    PQ openSet;
    int *closedSet;
    int nRcv=0, nSnt=0;
    int owner;
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

    while(1){
        // TODO: potremmo aggiungere una conditional variable: se sia M2S che openSet sono vuote
        //      (che è la condizone di terminazione parziale del singolo thread), il thread
        //      viene messo a dormire e viene svegliato quando il master aggiunge un messaggio
        //      nella sua coda, oppure quando il master si accorge che tutti i thread sono in
        //      in quella situazione quindi li sveglia tutti per farli terminare (quindi dopo il
        //      risveglio il singolo thread dovrebbe controllare una condizione per capire se deve 
        //      terminare).

        // update the values of messages sent and received
        *(arg->nMsgSnt) += nSnt;
        *(arg->nMsgRcv) += nRcv;
        nRcv = nSnt = 0;  // Zero the counter of received and sent message of this wave

        // special trick used to stop the slave if the is no things to do
        if(sem_trywait(arg->semToDo) != 0){
            // enter here only when we have to wait, that it means that we don't have any work to do => (We have to ask to the Master if the algorithm is ended)
            sem_post(arg->semM); // wake up the master because I'm empty, Mattern's Method need a last loop without messages
            sem_wait(arg->semToDo); // go to sleep
            sem_wait(arg->semM); // if it wasn't the last lap => Everything must be put as before
        }
        sem_post(arg->semToDo); // Reset to the previous valute, because in the loops we count the number of elements in the queue through the semaphores.
        
        if(*(arg->stop)){
            #ifdef TIME
              TIMERstopEprint(arg->timer);
            #endif
            pthread_exit(NULL);
        }

        while(!QUEUEisEmpty(arg->M2S)){
            //extract a message from the queue
            message = QUEUEheadExtract(arg->M2S);

            // decrement the number of thing that we have to do
            sem_wait(arg->semToDo); // must be non-blocking, since his construction
            nRcv++; // increment the received messages

            newGscore = message->priority;
            #ifdef DEBUG
                printf("%d: analyzing %d\n", arg->id, message->index);
            #endif
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
                    continue;
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
                        continue;
                    }
                        
                }
            }
            arg->path[message->index] = message->father;
            free(message); // a message is never freedzed
        }

        if(PQempty(openSet))
            continue;
        
        extrNode = PQextractMin(openSet);
        closedSet[extrNode.index] = extrNode.priority;
        // decrement the number of thing that we have to do
        sem_wait(arg->semToDo); // must be non-blocking, since his construction

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

            owner = hashing(t->v, arg->numTH);
            message = HITEMinit(t->v, newGscore, extrNode.index, owner, NULL);
            if(owner == arg->id){
                // avoid passing through the master
                QUEUEtailInsert(arg->M2S, message);
                sem_post(arg->semToDo);
            }else{
                QUEUEtailInsert(arg->S2M, message);
                sem_post(arg->semM); // tells the master that there is a message in the queue
            }
            nSnt++; // inrement the number of message sent
        }
    }


    //destroy openSet PQ
    PQfree(openSet);

    //destroy closedSet
    free(closedSet);

    pthread_exit(NULL);
}

static int hashing(int s, int numTH){
    return s%numTH;
}
