#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "Item.h"


Item* ITEMinit(int node_index, int priority){
  Item *item = (Item*) malloc(sizeof(*item));

  if(item == NULL){
    exit(1);
  }

  item->index = node_index;
  item->priority = priority;

  return item;
}

HItem HITEMinit(int index, int priority, int father, int owner, HItem next){
  HItem tmp = (HItem) malloc(sizeof(*tmp));

  if(tmp == NULL){
    perror("Error trying to allocate an HItem: ");
    exit(1);
  }

  tmp->father = father;
  tmp->index = index;
  tmp->next = next;
  tmp->owner = owner;
  tmp->priority = priority;

  return tmp;
}