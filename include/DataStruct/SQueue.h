/**
 * @file SQueue.h
 * @brief Header file of SQueue.c
 */

#ifndef SQueue_h
#define SQueue_h

#define MAX_STRING_NODE 1024 /**< Max length for node string representation. Used by #SQueue_print */

#include <stdio.h>
#include <errno.h>
#include <pthread.h>

typedef	void (*funDealloc)(void *); /**< function to dealloc data inside nodes */
typedef	void (*funMap)(void *); /**< function to applay to data contained in each node of the list */
typedef	int (*funCmp)(void *, void *); /**< function to compare data inside nodes */
typedef	void (*funPrint)(char *buf, size_t s, void * data); /**< function to print data inside nodes */

typedef struct SQueue SQueue;
typedef struct Node Node;
/**
 * @brief Queue node containing generic data (void *).
 */
struct Node {
    void * data; /**< generic data pointer */
    struct Node * next; /**<  pointer to next node object in the queue*/
};

/**
 * @brief SQueue is a mutable thread safe queue in which generic elements (void *) can be added or removed
 * 
 */
struct SQueue{
    pthread_mutex_t lock;  /**< lock variable */
    pthread_cond_t cv_full; /**< used to wait when is full */
    pthread_cond_t cv_empty; /**< used to wait when is empty */ 
    long n;  /**< number of element currently in the queue */
    long max;  /**< is the max number of elements that queue can contain (<=0: no limit) */
    Node * h; /**< head pointer */
    Node * t; /**< tail pointer */
};



SQueue * SQueue_init(long p_max);
int SQueue_deleteQueue(SQueue * p_q, funDealloc p_funNodeDel);
int SQueue_push(SQueue * p_q, void * p_new);
int SQueue_pushWait(SQueue * p_q, void * p_new);
int SQueue_pop(SQueue * p_q, void ** p_removed);
int SQueue_popWait(SQueue * p_q, void ** p_removed);
int SQueue_isEmpty(SQueue * p_q);
int SQueue_isFull(SQueue * p_q);
void SQueue_print(SQueue * p_q, funPrint p_funPrint);
int SQueue_dim(SQueue * p_q);
int SQueue_find(SQueue * p_q, void * p_target, funCmp p_funCmp);
int SQueue_remove(SQueue * p_q, void * p_target, funCmp p_funCmp);
int SQueue_removePos(SQueue * p_q, int p_pos, void ** p_removed);
void SQueue_map(SQueue * p_q, funMap p_funMap);

#endif /* SQueue_h */
