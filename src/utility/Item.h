#ifndef ITEM_H
#define ITEM_H

typedef struct item {
    int index;
    int priority;
} Item;

Item* ITEMinit(int node_index, int priority);

#endif