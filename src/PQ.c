#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "PQ.h"
#include "./utility/Item.h"
#include "sys/types.h"
#include <pthread.h>
#include <unistd.h>
#include "utility/Timer.h"
#include <time.h>

#define PARALLEL_SEARCH


struct pqueue { 
  Item *A; //array of Items.
  int heapsize; // number of elements in the priority queue
  int maxN; // max number of elements
};

#ifdef PARALLEL_SEARCH
/*This struct contains the result of the search and it is saved 
as a pointer inside SearchSpec, only the thread with a result will write it's fields*/
typedef struct search_res {
  int index;
  int priority;
} *SearchRes;

/*SearchSpec contains all the information needed by a thread to perform
 a search of a specific node_index inside the priority queue*/
typedef struct search_spec {
  int tid;
  int n_items;
  int start_index;
  int *search_complete;
  int thread_finished;
  int *target;
  PQ pq;
  pthread_mutex_t *found_mutex;
  pthread_cond_t *thread_wait_cv;
  pthread_cond_t *master_wait_cv;
  int *start_flag;
  int *found;
} SearchSpec;


static void *thread_search(void* arg);
#endif

/*
  Prototype declaration of static functions used.
*/
static void Swap(PQ pq, int n1, int n2);
static void Heapify(PQ pq, int i);
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


long max_thread;
SearchSpec **sp_array;
SearchRes sr; 
pthread_t *th;

PQ PQinit(int maxN) {
  PQ pq;
  
  //allocate space needed for the structure
  pq = malloc(sizeof(*pq));
  if(pq == NULL){
    perror("Error trying to allocate PQ structure: ");
    return NULL;
  }

  pq->A = malloc(maxN * sizeof(Item));
    if(pq->A == NULL){
    perror("Error trying to allocate heap array: ");
    return NULL;
  }
  pq->heapsize = 0;
  pq->maxN = maxN;

  #ifdef PARALLEL_SEARCH
    max_thread = sysconf(_SC_NPROCESSORS_ONLN) * 2;

    th = (pthread_t*) malloc(max_thread * sizeof(pthread_t));
    pthread_mutex_t *found_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_cond_t *thread_wait_cv = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    pthread_cond_t *master_wait_cv = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));

    pthread_mutex_init(found_mutex, NULL);
    pthread_cond_init(thread_wait_cv, NULL);
    pthread_cond_init(master_wait_cv, NULL);

    int *found = (int*) malloc(sizeof(int));
    *found = 0;

    int *target = (int*) malloc(sizeof(int));
    *target = -1;

    int *start_flag = (int*) malloc(sizeof(int));
    *start_flag = 0;

    int *search_complete = (int*) malloc(sizeof(int));
    *search_complete = 0;


    sr = (SearchRes) malloc(sizeof(sr));
    sr->index = -1;
    sp_array = (SearchSpec**) malloc(max_thread * sizeof(SearchSpec*));


    for(int i=0; i<max_thread; i++){
      SearchSpec *sp = (SearchSpec*) malloc(sizeof(SearchSpec));
      sp->tid = i;
      sp->found_mutex = found_mutex;
      sp->thread_wait_cv = thread_wait_cv;
      sp->master_wait_cv = master_wait_cv;
      sp->found = found;
      sp->start_flag = start_flag;
      sp->n_items = -1;
      sp->start_index = -1;
      sp->target = target;
      sp->pq = pq;
      sp->search_complete = search_complete;
      sp->thread_finished = 1;

      sp_array[i] = sp;

      

      // if((pq->heapsize >= max_thread) && (i == (actual_threads-1))){
      //   sp->n_items += rem;
      // }

      int res = pthread_create(&th[i], NULL, thread_search, (void*) sp_array[i]);

      // printf("Main: %d, Res: %d\n", i, res);
    }

  #endif

  return pq;
}


void PQterminate(){
  for(int i=0; i<max_thread; i++){
      pthread_join(th[i], NULL);
      free(sp_array[i]);
      free(sr);
  }

  
}

/*
  This function frees all memory used by the PQ, that is
  memory used fot the heap and for the PQ structure itself.

  Parameter: PQ to be freed.
*/
void PQfree(PQ pq) {

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

/*
  Return the dimension of the array of Items

  Parameters: PQ

  Return: the dimension of array pq->A
*/
int PQmaxSize(PQ pq){
  return pq->maxN;
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
void PQinsert (PQ pq, int node_index, int priority){
  Item *item = ITEMinit(node_index, priority);
  int i;
  
  // if( pq->heapsize >= pq->maxN){
  //   pq->A = realloc(pq->A, (2*pq->maxN)* sizeof(Item));
  //   if(pq->A == NULL){
  //     perror("Realloc");
  //     free(pq->A);
  //     free(pq);
  //     exit(1);
  //   }
  //   pq->maxN = 2*pq->maxN;
  // }

  //set i equal to the most-right available index. Also update the heap size.
  i = pq->heapsize++;

  //find the correct position of Item by performing the set of comparison
  while (i>=1 && ((pq->A[PARENT(i)]).priority > item->priority)) {
    pq->A[i] = pq->A[PARENT(i)];
    i = PARENT(i);
  }

  pq->A[i] = *item;
  
  free(item);
  return;
}

/*
  Swap element in position n1 in the heap with element in position n2
  and viceversa. Swapping them also implies changin thei entries in qp.

  Parameters: PQ, indexes of element to be swap.
*/
static void Swap(PQ pq, int n1, int n2){
  Item temp;

  temp  = pq->A[n1];
  pq->A[n1] = pq->A[n2];
  pq->A[n2] = temp;

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
static void Heapify(PQ pq, int i) {
  int l, r, smallest;

  //retrieve the index of the left child
  l = LEFT(i);

  //retrieve the index of the right child
  r = RIGHT(i);
  //compare the priority of the ith-node with the one of its left child
  if ( (l < pq->heapsize) && ( (pq->A[l]).priority < (pq->A[i]).priority) )
    smallest = l;
  else
    smallest = i;
  //compare the priority of the smallest with the one of its right child
  if ( (r < pq->heapsize) && ( (pq->A[r]).priority < (pq->A[smallest]).priority) )
    smallest = r;

  //if one of the children has an higher priority, swap it with the parent
  if (smallest != i) {
    Swap(pq, i,smallest);
	  Heapify(pq, smallest);
  }
  //If the root is smaller than the two children it will stop even if some nodes underneath dont respect heap condition
}

void main(void){
  PQ set = PQinit(1000000);
  //PQinsert(set, 2, 10);
  // PQinsert(set, 4, 12);
  // PQinsert(set, 6, 14);


  for(int i=0; i<1000000; i++){
    PQinsert(set, i, i*2);
  }

  int *prio = malloc(sizeof(int));
  int index;

  //index = PQsearch(set, 1, prio); 
  //index = PQsearch(set, 3, prio);

  index = PQsearch(set, 50000, prio);
  index = PQsearch(set, 123456, prio);
  index = PQsearch(set, 2000000, prio);

  // PQterminate();
}


/*
Each thread performs the search in a portion of the array pq->A,
if a match is found the thread sets the values in the struct SearchRes that
contains the index of the node and it's priority
*/

//TODO: CHECK IF ALL PARAMETERS ARE RECEIVED CORRECTLY AND CONTINUE DEBUGGING
#ifdef PARALLEL_SEARCH
static void *thread_search(void* arg){
  SearchSpec *sp = (SearchSpec*) arg;

  while(1){  
    //Wait for a search to be scheduled
    pthread_mutex_lock(sp->found_mutex);
    while((sp->thread_finished == 1) || (*(sp->start_flag) == 0)){
      printf("Thread %d waits for start_flag to be set to 1\n", sp->tid);

      int sc_bound = (sp->pq)->heapsize >= 16 ? max_thread : (sp->pq)->heapsize % 16;

      if(*(sp->search_complete) >= sc_bound){
        pthread_cond_signal(sp->master_wait_cv);
      }

      printf("Thread %d unlocked found mutex and goes waiting\n", sp->tid);
      pthread_cond_wait(sp->thread_wait_cv, sp->found_mutex);
    }

    pthread_mutex_unlock(sp->found_mutex);
    printf("Thread %d is woken up and starts searching, heapsize: %d\n", sp->tid, (sp->pq)->heapsize);

    
    // if(sp->tid >= (sp->pq)->heapsize){
    //   printf("Thread %d shouldn't search\n", sp->tid);
    //   pthread_mutex_lock(sp->found_mutex);
    //   *(sp->search_complete) += 1;
    //   pthread_mutex_unlock(sp->found_mutex);
    //   sp->thread_finished = 1;
    //   continue;
    // }
    // else{
       printf("Thread %d searches target %d from %d to %d, num items: %d\n", sp->tid, *(sp->target), sp->start_index, sp->start_index + sp->n_items, sp->n_items);
      for(int i=0; i<sp->n_items; i++){
        pthread_mutex_lock(sp->found_mutex);
        if(*(sp->found) == 1){
          printf("Thread %d, result has already been found by another thread\n", sp->tid);
          pthread_mutex_unlock(sp->found_mutex);
          break;
        }
        pthread_mutex_unlock(sp->found_mutex);

        
        if(((sp->pq)->A[sp->start_index+i]).index == *sp->target){
          pthread_mutex_lock(sp->found_mutex);

            sr->index = (sp->start_index+i);
            sr->priority = ((sp->pq)->A[sp->start_index+i]).priority;
            *sp->found = 1;
            *sp->start_flag = 0;

            printf("Thread %d found node, index: %d, priority: %d\n", sp->tid, sr->index, sr->priority);
            pthread_cond_signal(sp->master_wait_cv);

            
          pthread_mutex_unlock(sp->found_mutex);
          break;
        }
      }

      
      pthread_mutex_lock(sp->found_mutex);
      if(!(*sp->found)){
        printf("Thread %d couldn't find the target in it's range\n", sp->tid);
        *(sp->search_complete) += 1;
      }
      pthread_mutex_unlock(sp->found_mutex);
      sp->thread_finished = 1;
    // }
    
    //free(sp);
    // TIMERstopEprint(timer);
    
  }
}
#endif


/*
Searches for a specific node inside the Item array and returns it's index 
and the priority value inside the priority pointer
*/
int PQsearch(PQ pq, int node_index, int *priority){
  // printf("\n\nRequested search for node: %d\n", node_index);
  #ifndef PARALLEL_SEARCH
    //Timer timer = TIMERinit(1); 
    //TIMERstart(timer);
    int pos = -1;
    for(int i=0; i<pq->heapsize; i++){
      if(node_index == (pq->A[i]).index){
        if(priority != NULL){
          *priority = (pq->A[i]).priority;
        }
        pos = i;
        break;
      }
    }
    //TIMERstopEprint(timer);
    return pos;
    
  #endif

  /*Generates as many thread as the number of processors times 2.
    Each thread is assigned a portion of pq to search according to 
    the total number of items (pq->heapsize)
  */
  #ifdef PARALLEL_SEARCH
    printf("PQ search requested\n");
    
    long required_threads = max_thread;
    int index;
    int items_each;
    int rem;

    if(pq->heapsize == 0){
      printf("Heapsize is 0, returning. index: %d\n", sr->index);
      //free(sr);

      return -1;
    }
    else if(pq->heapsize < max_thread){
      printf("Heap size is less than number of threads, calling %d threads\n", pq->heapsize);
      items_each = 1;
      required_threads = pq->heapsize;

      for(int i=0; i<required_threads; i++){
        sp_array[i]->n_items = 1;
        sp_array[i]->start_index = i;
        sp_array[i]->thread_finished = 0;
        

        if(i == (required_threads-1)){
          sp_array[i]->pq = pq;
          *(sp_array[i]->start_flag) = 1; 
          *(sp_array[i]->target) = node_index;
          *(sp_array[i]->search_complete) = 0;

          //pthread_mutex_lock(sp_array[i]->found_mutex);
          // pthread_mutex_lock(sp_array[i]->start_mutex);
          //   pthread_cond_broadcast(sp_array[i]->thread_wait_cv);
          //   printf("Master broadcast thread\n");
          // pthread_mutex_unlock(sp_array[i]->start_mutex);
        }
      }


      //printf("Launching %ld threads\n", max_thread);
    }
    else {
      printf("Heap size is greater than max_thread, calling %ld threads\n", max_thread);
      required_threads = max_thread;
      items_each = pq->heapsize / max_thread;
      rem = pq->heapsize % max_thread;

      for(int i=0; i<max_thread; i++){
        sp_array[i]->n_items = items_each;
        sp_array[i]->start_index = i*items_each;
        sp_array[i]->thread_finished = 0;

        if(i == (max_thread-1)){
          sp_array[i]->pq = pq;
          sp_array[i]->n_items += rem;
          *(sp_array[i]->start_flag) = 1; 
          *(sp_array[i]->target) = node_index;
          *(sp_array[i]->search_complete) = 0;

          //pthread_mutex_lock(sp_array[i]->found_mutex);
          //printf("Master locks found\n");
          // pthread_mutex_lock(sp_array[i]->start_mutex);
          //   pthread_cond_broadcast(sp_array[i]->thread_wait_cv);
          //   printf("Master broadcast thread\n");
          // pthread_mutex_unlock(sp_array[i]->start_mutex);
        }
      }
    } 



    //Wait for a thread on a cv!
    
    //LOCK TO FOUND_MUTEX BEFORE LOCK TO START MUTEX BRINGS TO DEADLOCK
    //pthread_mutex_lock(sp_array[required_threads-1]->start_mutex);
    pthread_mutex_lock(sp_array[required_threads-1]->found_mutex);
    while((*(sp_array[required_threads-1]->found) == 0) && (*(sp_array[required_threads-1]->search_complete) < required_threads)){
      printf("Master broadcasts and awaits for a result\n");
      pthread_cond_broadcast(sp_array[required_threads-1]->thread_wait_cv);
      pthread_cond_wait(sp_array[required_threads-1]->master_wait_cv, sp_array[required_threads-1]->found_mutex);
    }

    pthread_mutex_unlock(sp_array[required_threads-1]->found_mutex);
    //pthread_mutex_unlock(sp_array[required_threads-1]->start_mutex);


    //We have the solution here



    //printf("heapsize: %d, items each: %d\n", pq->heapsize, items_each);
    


    if(priority != NULL){
      if(sr->index != -1){
        *priority = sr->priority;
      }
      else{
        sr->priority = -1;
      }
    }
    
    index = sr->index;

    
    printf("Search result: index=%d, priority=%d\n", sr->index, sr->priority);

    sr->index = -1;
    *(sp_array[0]->found) = 0;

    //free(sp);


    
    return index;

  #endif
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
Item PQextractMin(PQ pq) {
  Item item;

  if(pq->heapsize == 0){
    item.index = item.priority = -1;
    return item;
  }

  //swap the root with last element of the array
  Swap (pq, 0, pq->heapsize-1);
  //store the Item to return
  item = pq->A[pq->heapsize-1];
  //decrease the heapsize
  pq->heapsize--;
  //restore heap properties by calling HEAPify on the root
  Heapify(pq, 0);

  #ifdef DEBUG
    printf("\n*****AUXILIARY******\n");
    PQdisplayHeap(pq);
    printf("\n**************\n");
  #endif

  return item;
}

/*
  This function get the root Item without extracting it.
*/
Item PQgetMin(PQ pq){
  return pq->A[0];
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
void PQchange (PQ pq, int node_index, int priority) {
  int item_index = PQsearch(pq, node_index, NULL);
  Item item = pq->A[item_index];
  item.priority = priority;


  while( (item_index>=1) && ((pq->A[PARENT(item_index)]).priority > item.priority)) {
    pq->A[item_index] = pq->A[PARENT(item_index)];
	  item_index = PARENT(item_index);
  }

  pq->A[item_index] = item;
  Heapify(pq, item_index);

  return;
}

void PQdisplayHeap(PQ pq){
  for(int i=0; i<pq->heapsize; i++){
    printf("i = %d, priority = %d\n", (pq->A[i]).index, (pq->A[i]).priority);
  }
  return;
}

float PQgetPriority(PQ pq, int index){
  return (pq->A[index]).priority;
}







