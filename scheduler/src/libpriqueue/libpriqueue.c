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


node_t* node_init(void* data, node_t* next){
  node_t *node = malloc(sizeof(node_t));
  node->data = data;
  node->next = next;
  return node;
}

//Removes the node next to the supplied node from queue q. This is the 
//only way to remove an intermediate node in a single linked list.
//Returns the data of the deleted node
void* remove_next_node(priqueue_t *q, node_t* node){
    assert(q != NULL);
    assert(node != NULL);
    assert(q->size > 0);
    node_t *next = node->next;
    if(!next)
      return NULL;
    else{
      node->next = node->next->next;
      void* next_data = next->data;
      free(next);
      q->size--;
      if(!node->next)
        q->tail = node;
      return next_data;
    }
}

/**
  Removes the head of the queue, making the next node in the queue the new head.
  Returns the data pointer from the old head
*/
void* remove_queue_head(priqueue_t* q){
    node_t *head = q->head;
    q->head = head->next;
    void* head_data = head->data;
    free(head);
    q->size--;
    return head_data;
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
    if(q->size == 0)
        q->head =  q->tail = node_init(ptr, NULL);
    else{
      if(q->comparer(ptr, q->head->data) <= 0) //inserting to the head of the queue
          q->head = node_init(ptr, q->head);
      else{
        node_t *node;
        for(node = q->head; node->next != NULL; node = node->next){ //inserting elsewhere in the queue
          index++;
          if(q->comparer(ptr, node->next->data) <= 0){ //insert when comparer(a,b) gives a <= b (a-b <= 0)
              node->next = node_init(ptr, node->next);
              break;
          }
        }
        if(!node->next){  //inserting to the tail of the queue
            index++;
            node->next = node_init(ptr, NULL);
            q->tail = node->next;
        }        
      }
    }
    q->size++;
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
    return priqueue_remove_at(q,q->size-1); //head index is 0, so tail is at size-1

}


/**
  Much like priqueue_at, but returns the node, instead of the data in it.
*/

node_t *priqueue_node_at(priqueue_t *q, int index)
{
  assert(q != NULL);
  if(index >= q->size || index < 0)
	  return NULL;
  else{
    node_t* node = q->head;
    for(int i = 0 ; i < index; node = node->next, ++i);
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
  if(q->size > 0){
    for(node_t* node = q->head; node->next != NULL;){
      if(node->next->data == ptr){
        remove_next_node(q,node);
        entries_removed++;
      }
      else
        node = node->next;
    }
    if(q->head->data == ptr){
      remove_queue_head(q);
      entries_removed++;
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
  if(index - 1 < 0)
    return remove_queue_head(q);
  return remove_next_node(q, priqueue_node_at(q,index-1));
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
  for(node_t* node = q->head; node->next != NULL;){
    remove_next_node(q, node);    
  }
  free(q->head);
}
