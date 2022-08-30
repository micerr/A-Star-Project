#ifndef ITEM_H
#define ITEM_H

typedef struct item {
    int index;
    int priority;
} Item;

typedef struct hitem *HItem;
struct hitem{
    int index;
    int priority;
    int father;
    int owner;
    HItem next;
} ;


Item* ITEMinit(int node_index, int priority);
HItem HITEMinit(int index, int priority, int father, int owner, HItem next);

#endif