#include <stdlib.h>
#include <stdio.h>
#include "PQ.h"

//struct representing the priority queue
struct pqueue { 
  int *A;         //array of indexes (int). Items are referred to using indexes
  int *qp;        //array containing heap position of each Item
  int heapsize;   //number of elements int the priority queue
};

/*
  Prototype declaration of static functions used.
*/
static void Swap(PQ pq, int n1, int n2);
static void Heapify(PQ pq, int *priority, int i);
static int LEFT(int i);
static int RIGHT(int i);
static int PARENT(int i);

/*
  This function allows to retrieve the position of the i-node's left child
  within the array representing the heap .

  Parameter: position of the node within the array.

  Return: position of the left child
*/
static int LEFT(int i) {
  return i*2+1;
}

/*
  This function allows to retrieve the position of the i-node's right child
  within the array representing the heap .

  Parameter: position of the node within the array.

  Return: position of the right child
*/
static int RIGHT(int i) {
  return i*2+2;
}

/*
  This function allows to retrieve the position of the i-node's parent
  within the array representing the heap .

  Parameter: position of the node within the array.

  Return: position of the parent node
*/
static int PARENT(int i) {
  return (i-1)/2;
}

/*
  This function allocates space needed for the structure representing
  the priority queue and for the array used as an heap.

  Parameters: maximum number of elements that the PQ will contain.

  Return: a pointer to a structure representing the PQ in case of success,
  a NULL pointer otherwise.
*/
PQ PQinit(int maxN) {
  int i;
  PQ pq;
  
  //allocate space needed for the structure
  pq = malloc(sizeof(*pq));
  if(pq == NULL){
    perror("Error trying to allocate PQ structure: ");
    return NULL;
  }

  //allocate the heap (array of indexes)
  pq->A = malloc(maxN*sizeof(int));
  if(pq == NULL){
    perror("Error trying to allocate heap array: ");
    return NULL;
  }

  //allocate the array containing heap position of each Item
  pq->qp = malloc(maxN*sizeof(int));
  if(pq == NULL){
    perror("Error trying to allocate PQ structure: ");
    return NULL;
  }

  //set all heap entries as free (not occupied)
  for (i=0; i < maxN; i++)
    pq->qp[i] = -1;

  //initialize number of elements in the array to 0
  pq->heapsize = 0;
  return pq;
}

/*
  This function frees all memory used by the PQ, that is
  memory used fot the heap and for the PQ structure itself.

  Parameter: PQ to be freed.
*/
void PQfree(PQ pq) {
  free(pq->qp);
  free(pq->A);
  free(pq);
}

/*
  This function checks if the priority queue is empty by comparing
  the value heapsize with zero.

  Parameters: PQ.

  Return: the result of the comparison.
*/
int PQempty(PQ pq) {
  return pq->heapsize == 0;
}

// Inside the while loop, the position of two nodes in the heap is exchanged
// This happens because the node we want to insert has a lower priority value
// wrt his parent. In addition to the exchange of the nodes, also the qp
// vector has to be updated (qp contains the index of a node inside the heap)

/*
  This function allows to insert a new Item in the heap so in the PQ.
  Each time a new Item is inserted, it should be positioned in the most-right
  position of the heap. Before positioning, new Item's priority is checked
  against the one of its parent. If the priority is lower or equal, its index
  is stored in the selected position, otherwise (higher priority) its parent
  is moved down in the heap, anc the priority of the new Item is checkeg against
  the one of the parent of its parent. This set of comparison id performed in
  a while loop until the heap's root is reached or untill a parent node with an
  higher priority is found. Every swap action implies changing also the 
  corresponding entry in the qp array.

  Parameters: PQ in which we want to insert the new node, array containing
  priority of each Item in the heap, Item's index used to retrieve its priority.
*/
void PQinsert (PQ pq, int *priority, int node){
  int i;

  //set i equal to the most-right available index. Also update the heap size.
  i = pq->heapsize++;

  pq->qp[node] = i;

  //find the correct position of Item by performing the set of comparison
  while (i>=1 && (priority[pq->A[PARENT(i)]] > priority[node])) {
    pq->A[i] = pq->A[PARENT(i)];
    pq->qp[pq->A[i]] = i;
    i = PARENT(i);
  }
  pq->A[i] = node;
  pq->qp[node] = i;
  return;
}

/*
  Swap element in position n1 in the heap with element in position n2
  and viceversa. Swapping them also implies changin thei entries in qp.

  Parameters: PQ, indexes of element to be swap.
*/
static void Swap(PQ pq, int n1, int n2){
  int temp;

  temp = pq->A[n1];
  pq->A[n1] = pq->A[n2];
  pq->A[n2] = temp;

  n1 = pq->A[n1];
  n2 = pq->A[n2];
  temp = pq->qp[n1];
  pq->qp[n1] = pq->qp[n2];
  pq->qp[n2] = temp;

  return;
}

/*
  This function verify that the heap functional property is satisfied
  and eventually restore it.
  The priority of a node is compared against with those of its children:
  If one child has an higher priority, swap the parent with the child and
  call Heapify recursivelly on the child. Important assumption is that
  sub-trees rooted on the 2 children must be 2 heap.

  Parameters: PQ, array containing nodes' priority, node where to start
  checking.
*/
static void Heapify(PQ pq, int *priority, int i) {
  int l, r, smallest;

  //retrieve the index of the left child
  l = LEFT(i);

  //retrieve the index of the right child
  r = RIGHT(i);

  //compare the priority of the ith-node with the one of its left child
  if (l < pq->heapsize && (priority[pq->A[l]] < priority[pq->A[i]]))
    smallest = l;
  else
    smallest = i;

  //compare the priority of the smallest with the one of its right child
  if (r < pq->heapsize && (priority[pq->A[r]] < priority[pq->A[smallest]]))
    smallest = r;

  //if one of the children has an higher priority, swap it with the parent
  if (smallest != i) {
    Swap(pq, i,smallest);

    //call heapify recursivelly
	  Heapify(pq, priority, smallest);
  }
  return;
}

/*
  This function allows to retrieve the index of the Item with the highest
  priority, that is the first element in the heap array. The extraction is
  done by swapping the root with the last element in the array, saving the
  index in the last element (that is the old root), decreasing the heapsize
  and eventually restoring the heap functional property by calling HEAPify.

  Parameters: PQ and nodes' priority array.

  Return: the index of the node with the highest priority.
*/
int PQextractMin(PQ pq, int *priority) {
  int k;

  //swap the root with last element of the array
  Swap (pq, 0, pq->heapsize-1);

  //store the index to return
  k = pq->A[pq->heapsize-1];

  //decrease the heapsize
  pq->heapsize--;

  //restore heap properties by calling HEAPify on the root
  Heapify(pq, priority, 0);
  return k;
}

/*
  This function allows changing the priority of an Item. The position
  of the Item within the heap must be known and passed as parameter.

  To properly ensure the maintenance of the heap property, before placing
  the Item with the new prioirty, it is checked that priority is lower
  than the ones of its parent and/or ancestors; if the new priority is
  higher, ancestors are moved down by mean of swapping operations untill
  a node with higher priority is found as parent or untille the root is 
  reached. After having found the correct position, HEAPify is applied
  to restore heap properites.

  OSS: HEAPify call is effective only if the new priority is lower than
  the previous one.

  Parameters: PQ, Items' priority array and position of the Item we want
  to change priority. 
*/
void PQchange (PQ pq, int *priority, int k) {
  int pos = pq->qp[k];
  int temp = pq->A[pos];

  while (pos>=1 && (priority[pq->A[PARENT(pos)]] > priority[pq->A[pos]])) {
    pq->A[pos] = pq->A[PARENT(pos)];
    pq->qp[pq->A[pos]] = pos;
    pos = (pos-1)/2;
  }
  pq->A[pos] = temp;
  pq->qp[temp] = pos;

  Heapify(pq, priority, pos);
  return;
}


