#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>

#include "Queue.h"

struct queue{
    HItem head;
    HItem tail;
    pthread_mutex_t *meh, *met;
};

Queue QUEUEinit(){
    Queue q = (Queue) malloc(sizeof(*q));
    HItem dummy = HITEMinit(-1, INT_MAX, -1, -1, NULL);

    if(q == NULL){
        perror("Error trying to allocate a queue: ");
        exit(1);
    }

    q->meh = malloc(sizeof(pthread_mutex_t));
    q->met = malloc(sizeof(pthread_mutex_t));
    if(q->meh == NULL || q->met == NULL){
        perror("Error trying to allocate a queue's lock: ");
        exit(1);
    }

    q->head = dummy;
    q->tail = dummy;
    pthread_mutex_init(q->meh, NULL);
    pthread_mutex_init(q->met, NULL);

    return q;
}
void QUEUEfree(Queue q){
    HItem t, toBeFreed;

    for(t=q->head; t!=NULL; ){
        toBeFreed = t;
        t=t->next;
        free(toBeFreed);        
    }
    pthread_mutex_destroy(q->meh);
    pthread_mutex_destroy(q->met);
    free(q->meh);
    free(q->met);
    free(q);
}

void QUEUEtailInsert(Queue queue, HItem node){
    node->next = NULL;
    pthread_mutex_lock(queue->met);
    queue->tail->next = node;
    queue->tail = node;
    pthread_mutex_unlock(queue->met);
}

HItem QUEUEheadExtract(Queue queue){
    HItem t;
    pthread_mutex_lock(queue->meh);

    if(queue->head->next == NULL){
        // empty queue
        pthread_mutex_unlock(queue->meh);
        return NULL;
    }

    pthread_mutex_lock(queue->met);
    if(queue->head->next == queue->tail){
        // last element in the queue
        t = queue->tail;
        queue->head->next = NULL;
        queue->tail = queue->head;

        pthread_mutex_unlock(queue->met);
        pthread_mutex_unlock(queue->meh);
        return t;
    }
    pthread_mutex_unlock(queue->met);

    t = queue->head->next;

    queue->head->next = t->next;

    pthread_mutex_unlock(queue->meh);
    return t;
}

void QUEUEprint(Queue queue){
    HItem t;

    for(t=queue->head; t!=NULL; t=t->next){
        printf("index: %d\n", t->index);
        printf("priority: %d\t->\n", t->priority);
        printf("father: %d\n\n", t->father);
    }
}

int QUEUEisEmpty(Queue queue){
    int ret;
    pthread_mutex_lock(queue->meh);
    ret = (queue->head->next == NULL);
    pthread_mutex_unlock(queue->meh);
    return ret;
}

int QUEUEsAreEmpty(Queue *queues, int n, int *firstNotEmpty){
    int i, empty=1;
    *firstNotEmpty = 0;
    for(i=0; i<n; i++)
        if(queues[i] != NULL)
            if(!QUEUEisEmpty(queues[i])){
                empty = 0;
                *firstNotEmpty = i;
                continue;
            }
    return empty;
}

HItem QUEUEsHeadExtract(Queue *queues, int n, int startFrom){
    int i, j;
    HItem t;
    for(i=startFrom; i<n+startFrom; i++){
        j = i%n;
        if(queues[j] != NULL)
            if((t = QUEUEheadExtract(queues[j])) != NULL)
                break;
    }
    return t;
}
