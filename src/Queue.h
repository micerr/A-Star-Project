#ifndef QUEUH_H
#define QUEUE_H

#include "./utility/Item.h"


typedef struct queue* Queue;

Queue QUEUEinit();
void QUEUEfree(Queue q);
void QUEUEtailInsert(Queue queue, HItem node);
HItem QUEUEheadExtract(Queue queue);
HItem QUEUEsHeadExtract(Queue *queues, int n, int startFrom);
void QUEUEprint(Queue queue);
int QUEUEisEmpty(Queue queue);
int QUEUEsAreEmpty(Queue *queues, int n, int *firstNotEmpty);

#endif

