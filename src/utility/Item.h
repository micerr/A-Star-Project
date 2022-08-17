#ifndef ITEM_H
#define ITEM_H

typedef struct item {
    int index;
    float priority;
} Item;

Item* ITEMinit(int node_index, float priority);

#endif