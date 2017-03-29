/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_


/**
  Node structure for the Priqueue - Double Linked
*/

typedef struct node_t
{
  void* data;
  struct node_t* next;
  struct node_t* prev;
} node_t;


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

void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));
int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek_head(priqueue_t *q);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll_head(priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
void * priqueue_at       (priqueue_t *q, int index);
node_t *priqueue_node_at(priqueue_t *q, int index);
int    priqueue_remove   (priqueue_t *q, void *ptr);
void * priqueue_remove_at(priqueue_t *q, int index);
int    priqueue_size     (priqueue_t *q);

void   priqueue_destroy  (priqueue_t *q);

void swap(priqueue_t *q, int index1, int index2);

#endif /* LIBPQUEUE_H_ */
