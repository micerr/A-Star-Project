#ifndef ITEM_H
#define ITEM_H

typedef struct item {
    int index;
    int priority;
} Item;

typedef struct HItem *HItem_ptr;
struct HItem{
    int index;
    int priority;
    int father;
    HItem_ptr next;
} ;


Item* ITEMinit(int node_index, int priority);
HItem_ptr HITEMinit(int index, int priority, int father, HItem_ptr next);

#endif