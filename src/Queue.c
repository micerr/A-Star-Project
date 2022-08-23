#include <stdio.h>
#include <stdlib.h>

#include "Queue.h"

struct queue{
    int size;
    HItem_ptr head;
    HItem_ptr tail;
    HItem_ptr z;
};

Queue QUEUEinit(){
    Queue q = (Queue) malloc(sizeof(*q));

    if(q == NULL){
        perror("Error trying to allocate a queue: ");
        exit(1);
    }

    q->size = 0;
    q->z = HITEMinit(-1, -1, -1, NULL);
    q->head = q->z;
    q->tail = q->z;

    return q;
}
void QUEUEfree(Queue q){
    HItem_ptr t, toBeFreed;

    for(t=q->head; t!=q->z; ){
        toBeFreed = t;
        t=t->next;
        free(toBeFreed);        
    }

    free(q);
}

void QUEUEtailInsert(Queue queue, HItem_ptr node){
    queue->tail->next = node;
    queue->tail = node;
    node->next = queue->z;
}

HItem_ptr QUEUEheadExtract(Queue queue){
    HItem_ptr t;
    t = queue->head;
    queue->head = queue->head->next;

    return t;
}

void QUEUEprint(Queue queue){
    HItem_ptr t;

    for(t=queue->head; t!=queue->z; t=t->next){
        printf("index: %d\n", t->index);
        printf("priority: %d\n", t->priority);
        printf("father: %d\n", t->father);
        printf("\t\t->\n");
    }
}











