#include <stdio.h>
#include <stdlib.h>

#include "Queue.h"

struct queue{
    HItem head;
    HItem tail;
};

Queue QUEUEinit(){
    Queue q = (Queue) malloc(sizeof(*q));

    if(q == NULL){
        perror("Error trying to allocate a queue: ");
        exit(1);
    }

    q->head = NULL;
    q->tail = NULL;

    return q;
}
void QUEUEfree(Queue q){
    HItem t, toBeFreed;

    for(t=q->head; t!=NULL; ){
        toBeFreed = t;
        t=t->next;
        free(toBeFreed);        
    }

    free(q);
}

void QUEUEtailInsert(Queue queue, HItem node){
    node->next = NULL;
    if(queue->head == NULL){
        queue->head = queue->tail = node;
        return;
    }
    queue->tail->next = node;
    queue->tail = node;
}

HItem QUEUEheadExtract(Queue queue){
    HItem t;
    t = queue->head;

    if(queue->head == NULL)
        return t;
    queue->head = queue->head->next;
    if(queue->head == NULL)
        queue->tail = queue->head;

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
    return queue->head == NULL;
}









