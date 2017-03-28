/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_


/**
  Node structure for the Priqueue - Single Linked
*/

typedef struct node_t
{
  void* data;
  struct node_t* next;
} node_t;

node_t* node_init(void* data, node_t* next);

/**
  Priqueue Data Structure
*/

typedef struct _priqueue_t
{
  node_t* head; //pointers to other Jobs
  node_t* tail;
  int (*comparer)(const void*, const void*);
  int size;

} priqueue_t;

node_t* node_init(void* data, node_t* next);

void* remove_next_node(priqueue_t *q, node_t* node);
void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));
int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
void * priqueue_at       (priqueue_t *q, int index);
node_t *priqueue_node_at(priqueue_t *q, int index);
int    priqueue_remove   (priqueue_t *q, void *ptr);
void * priqueue_remove_at(priqueue_t *q, int index);
void*  remove_queue_head(priqueue_t* q);
int    priqueue_size     (priqueue_t *q);

void   priqueue_destroy  (priqueue_t *q);

void swap(priqueue_t *q, int index1, int index2);

#endif /* LIBPQUEUE_H_ */
