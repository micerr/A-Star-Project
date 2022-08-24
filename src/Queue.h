#include "./utility/Item.h"


typedef struct queue* Queue;

Queue QUEUEinit();
void QUEUEfree(Queue q);
void QUEUEtailInsert(Queue queue, HItem_ptr node);
HItem_ptr QUEUEheadExtract(Queue queue);
void QUEUEprint(Queue queue);


