/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "libpriqueue.h"




//This queue is implemented as a single linked list. 
//deletion is done from the tail side.
/*insertion starts from the head side, but may occur at an intermediate node 
  in the queue depending on the result of the comparator function at every node.*/
//Diagram:   Head -> . -> . -> .... -> . -> Tail => NULL


node_t* node_init(void* data, node_t* prev, node_t* next){
  node_t *node = malloc(sizeof(node_t));
  node->data = data;
  node->prev = prev;
  node->next = next;
  return node;
}

void* remove_node(priqueue_t* q, node_t* n){
    assert(q != NULL);
    assert(n != NULL);
    assert(q->size > 0);
    if(n->prev)
        n->prev->next = n->next;
    else
        q->head = n->next;  
    if(n->next)
        n->next->prev = n->prev;
    else
        q->tail = n->prev;
    void* n_data = n->data;
    free(n);
    q->size--;
    return n_data;
}

//inserts a new node with data 'data' before node 'n' (in queue 'q') (relative to the tail of q)
//if n is null, it is assumed we are inserting to the head of the queue
void insert_node(priqueue_t* q, void* data, node_t* n){
  assert(q != NULL);
  node_t *new_node;  
  if(n){
    new_node = node_init(data, n, n->next);
    if(n->next)
      n->next->prev = new_node;
    else //we are at the tail
      q->tail = new_node;
    n->next = new_node;     
  }
  else{ //we are at the head
    new_node = node_init(data, NULL, q->head);
    if(q->size == 0)
      q->tail = q->head = new_node;
    else{
      q->head->prev = new_node;
      q->head = new_node;
    }      
  }
  q->size++;
}


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
  q->tail = NULL;
  q->head = NULL; 
  q->comparer = comparer;
  q->size = 0;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
    assert(q != NULL);
    int index = 0;
    if(q->size == 0){
      insert_node(q, ptr, NULL);
    }
    else{
      for(node_t *node = q->tail; node != NULL; node = node->prev){
          index++;
          if(q->comparer(ptr, node->data) <= 0){ //insert when comparer(a,b) gives a <= b (a-b <= 0)
            insert_node(q, ptr, node);
            return index;
          }
      }
      index++;
      insert_node(q, ptr, NULL);
    }
	  return index;
}


/**
  Retrieves, but does not remove, the tail of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the tail of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	assert(q != NULL);
  if(q->size == 0)
    return NULL;
  else
    return q->tail->data;
}

/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek_head(priqueue_t *q)
{
	assert(q != NULL);
  if(q->size == 0)
    return NULL;
  else
    return q->head->data;
}


//swaps the data at the given indices, relative to the tail of q 
void swap(priqueue_t *q, int index1, int index2){
  assert(q != NULL);
  if(index1 > index2){
    swap(q, index2, index1);
  }
  else{
    assert(index1 >= 0 && index2 < q->size);
    node_t *node1, *node2; 
    int count = 0;
    for(node_t* node = q->tail;; node = node->prev, ++count){
      if(count == index1)
        node1 = node;
      else if(count == index2){
        node2 = node;
        break;
      }
    }
    void* temp = node1->data;
    node1->data = node2->data;
    node2->data = temp;
  }
}


/**
  Retrieves and removes the tail of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the tail of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
  assert(q != NULL);
  if(q->size == 0)
	  return NULL;
  else
    return remove_node(q, q->tail);
}

/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll_head(priqueue_t *q)
{
  assert(q != NULL);
  if(q->size == 0)
	  return NULL;
  else
    return remove_node(q, q->head);
}


/**
  Used by priqueue_at. Returns the node at the given index (relative to the tail)
*/

node_t *priqueue_node_at(priqueue_t *q, int index)
{
  assert(q != NULL);
  if(index >= q->size || index < 0)
	  return NULL;
  else{
    node_t* node = q->tail;
    for(int i = 0 ; i < index; node = node->prev, ++i);
    return node;
  }
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
  node_t* node = priqueue_node_at(q, index);
  if(!node)
    return NULL;
  return node->data;
}



/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{  
  assert(q != NULL);
  int entries_removed = 0;
  node_t* next_node;
  if(q->size > 0){
    for(node_t* node = q->head; node != NULL; node = next_node){
      next_node = node->next;
      if(node->data == ptr){
        remove_node(q, node);
        entries_removed++;
      }
    }
  }
	return entries_removed;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	assert(q != NULL);
  if(index < 0 || index >= q->size)
    return NULL;
  return remove_node(q, priqueue_node_at(q,index));
}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
  assert(q != NULL);
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
  assert(q != NULL);
  node_t *next_node;
  for(node_t* node = q->head; node != NULL; node = next_node){
    next_node = node->next;
    remove_node(q, node);    
  }
}
