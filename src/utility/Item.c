#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "Item.h"


Item* ITEMinit(int node_index, float priority){
  Item *item = (Item*) malloc(sizeof(*item));

  if(item == NULL){
    exit(1);
  }

  item->index = node_index;
  item->priority = priority;

  return item;
}
