#include <stdio.h>
#include <stdlib.h>

#include "Queue.h"

struct queue{
    int size;
    HItem_ptr head;
    HItem_ptr tail;
};

Queue QUEUEinit(){
    Queue q = (Queue) malloc(sizeof(*q));

    if(q == NULL){
        perror("Error trying to allocate a queue: ");
        exit(1);
    }

    q->size = 0;
    q->head = NULL;
    q->tail = NULL;

    return q;
}
void QUEUEfree(Queue q){
    HItem_ptr t, toBeFreed;

    for(t=q->head; t!=NULL; ){
        toBeFreed = t;
        t=t->next;
        free(toBeFreed);        
    }

    free(q);
}

void QUEUEtailInsert(Queue queue, HItem_ptr node){
    if(queue->size == 0){
        queue->head = node;
        queue->tail = node;
        queue->size++;
        return;
    }
    queue->tail->next = node;
    queue->tail = node;
    queue->size++;
}

HItem_ptr QUEUEheadExtract(Queue queue){
    HItem_ptr t;
    t = queue->head;

    if(queue->head == NULL)
        queue->tail = NULL;
    else
        queue->head = queue->head->next;

    queue->size--;
    return t;
}

void QUEUEprint(Queue queue){
    HItem_ptr t;

    for(t=queue->head; t!=NULL; t=t->next){
        printf("index: %d\n", t->index);
        printf("priority: %d\t->\n", t->priority);
        printf("father: %d\n\n", t->father);
    }
}











