/**
 * @file SQueue.h
 * @brief Header file of SQueue.c
 */

#ifndef SQueue_h
#define SQueue_h

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
/**
 * @brief Queue node containing generic data (void *).
 */
typedef struct _Node {
    void * data; /**< generic data pointer */
    struct _Node * next; /**<  pointer to next node object in the queue*/
} Node;

typedef struct _SQueue{
    pthread_mutex_t lock;  /**< lock variable */
    pthread_cond_t cv_full; /**< used to wait when is full */
    pthread_cond_t cv_empty; /**< used to wait when is empty */ 
    
    long n;  /**< number of element currently in the queue */
    long max;  /**< is the max number of elements that queue can contain (<=0: no limit) */
    Node * h; /**< head pointer */
    Node * t; /**< tail pointer */
} SQueue;

typedef	void funDealloc(void *); /**< function to dealloc data inside nodes */
typedef	void funPrint(void *); /**< function to print data inside nodes */

SQueue * SQueue_init(long p_max);
int SQueue_deleteQueue(SQueue * p_q, funDealloc p_funDealloc);
int SQueue_push(SQueue * p_q, void * p_new);
int SQueue_pushWait(SQueue * p_q, void * p_new);
int SQueue_pop(SQueue * p_q, void ** p_removed);
int SQueue_popWait(SQueue * p_q, void ** p_removed);
int SQueue_isEmpty(SQueue * p_q);
int SQueue_isFull(SQueue * p_q);
void SQueue_print(funPrint p_funPrint);

#endif /* SQueue_h */
