#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "AStar.h"
#include "Queue.h"

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
    Queue S2M;
    Queue M2S;
} slaveArg_t;

static void *masterTH(void *par);
static void *slaveTH(void *par);
static int multiplicativeHashing(int s);

void ASTARhda(Graph G, int start, int end, int numTH, int (*h)(Coord, Coord)){
    Queue *queueArr_S2M;
    Queue *queueArr_M2S;
    masterArg_t masterArg;
    slaveArg_t *slaveArgArr;

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

    printf("Slave%d has been started: numTH=%d - sleep for 1 second\n", arg->id, arg->numTH);
    sleep(1);

    pthread_exit(NULL);
}

static int multiplicativeHashing(int s){
    return 1;
}
