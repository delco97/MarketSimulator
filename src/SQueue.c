/**
 * @file SQueue.c
 * @author Andrea Del Corto
 * @brief   A SQueue is a mutable thread safe queue in which generic elements (void *) can be added or removed.
 *          A typical SQueue (abstract rappresentation):
 *              - <x1,x2,...,xn>: elements in queue
 *              - n: number of element currently in the queue
 *              - x1: is the tail element
 *              - xn: is the head element
 *              - max: maximum number of elements allowed in the queue (<=0: means no limit)
 */

#include <util.h>
#include <SQueue.h>
#include <pthread.h>
#include <stdio.h>


//Private fun
static Node * pSQueue_allocateNode() { return malloc(sizeof(Node));}
static SQueue * pSQueue_allocQueue() { return malloc(sizeof(SQueue)); }
static void pSQueue_freeNode(Node * n, funDealloc p_funDealloc) { p_funDealloc(n->data); free(n); }
static int pSQueue_isEmpty(SQueue * p_q);
static int pSQueue_isFull(SQueue * p_q);
static int pSQueue_pop(SQueue * p_q, void ** p_removed);
static int pSQueue_push(SQueue * p_q, void * p_new);
static void pSQueue_Lock(SQueue * p_q) {if(pthread_mutex_lock(&p_q->lock) != 0) err_quit("An error occurred during locking.");}
static void pSQueue_Unlock(SQueue * p_q) {if(pthread_mutex_unlock(&p_q->lock) != 0) err_quit("An error occurred during unlocking.");}
static void pSQueue_WaitFull(SQueue * p_q) {if(pthread_cond_wait(&p_q->cv_full, &p_q->lock) != 0) err_quit("An error occurred during cond wait.");}
static void pSQueue_WaitEmpty(SQueue * p_q) {if(pthread_cond_wait(&p_q->cv_empty, &p_q->lock) != 0) err_quit("An error occurred during cond wait.");}
static void pSQueue_SignalEmpty(SQueue * p_q) {if(pthread_cond_signal(&p_q->cv_empty) != 0) err_quit("An error occurred during singal empty.");}
static void pSQueue_SignalFull(SQueue * p_q) {if(pthread_cond_signal(&p_q->cv_full) != 0) err_quit("An error occurred during singal full.");}

/**
 * @brief Make a new empty queue.
 * @param p_max is the maximum number of elements for the queue, in particular: if 
 *              if p_max > 0 p_max set the maximum number of elements in the queue; otherwise
 *              there is no limit.
 * @return SQueue* pointer to new queue allocated, NULL if a probelm occurred during allocation.
 */
SQueue * SQueue_init(long p_max){    
    SQueue * aux = NULL;
    
    aux = (SQueue *)malloc(sizeof(SQueue));
    if(aux != NULL){
        aux->max = p_max;
        aux->n = 0;
        aux->h = pSQueue_allocateNode();
        if(aux->h == NULL) goto err;
        aux->h->data = NULL;
        aux->h->next = NULL;        
        aux->t = aux->h;
        //Locking system setup
        if (pthread_mutex_init(&(aux->lock), NULL) != 0 || 
            pthread_cond_init(&(aux->cv_empty), NULL) != 0 ||
            pthread_cond_init(&(aux->cv_full), NULL) != 0 )  goto err;
    }
    return aux;
err:
    if(aux != NULL){
        if(aux->h != NULL) free(aux->h);
        free(aux);
    }
    return NULL;
}
/**
 * @brief Dealloc a SQueue object.
 * 
 * @warning This function should be called by only one thread when no other thread is working on SQueue object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_funDealloc is the function used to deallocated data contained in all nodes.
 *        If p_funDealloc == NULL, data inside nodes will not be deallocated (usefull if data contained by nodes doesn't need to be deallocated).
 * @return int: result code:
 *  1: p_q != NULL and the deallocation proceed witout errors. 
 *  -1: p_q == NULL
 */
int SQueue_deleteQueue(SQueue * p_q, funDealloc p_funDealloc){
    if(p_q == NULL) return -1;
    Node * aux = NULL;
    while (p_q->h != NULL) {
        //Deallocated data inside node
        if(p_funDealloc != NULL) p_funDealloc(p_q->h->data);
        //Continue SQueue scan
        aux = p_q->h;
        p_q->h= p_q->h->next;
        //Deallocate Node
        pSQueue_freeNode(aux);
    }
    return 1;
}

/**
 * @brief Add a new element p_new in queue p_q only if the queue is not full.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_new New data to add.
 * @return int: result code:
 *  1: good
 *  -1: invalid pointer p_q
 *  -2: p_q is full
 *  -3: an error occurred during Node creation
 */
int SQueue_push(SQueue * p_q, void * p_new){
    int res_fun = 0;
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);
    res_fun = pSQueue_push(p_q, p_new);
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief  Add a new element p_new in queue p_q only if the queue is not full.
 *         If queue is full, it waits until there is space to add the new element.
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_new New data to add.
 * @return int: result code:
 *  1: good
 *  -1: invalid pointer p_q
 */
int SQueue_pushWait(SQueue * p_q, void * p_new){
    int res_fun = 0;
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);

    while (pSQueue_isFull(p_q) == 1) pSQueue_WaitFull(p_q);

    if( (res_fun = pSQueue_push(p_q, p_new)) == 1 )
        pSQueue_SignalEmpty(p_q);
    
    pSQueue_Unlock(p_q);
    return res_fun;
}

static int pSQueue_push(SQueue * p_q, void * p_new){
    int res_fun = 0;
    if(pSQueue_isFull(p_q) == 1)//Is full
        res_fun = -2;
    else{
        //add in head
        Node * aux = pSQueue_allocateNode();
        if(aux == NULL) return -3;
        aux->data = p_new;
        aux->next = NULL;
        p_q->t->next = aux;
        p_q->t = aux;
        p_q->n++;
        res_fun = 1;
    }
    return res_fun;
}

/**
 * @brief Remove element from p_q and put it in p_removed
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_removed Requirements: p_removed != NULL. This memory location will contains the removed element.
 * @return int: result code:
 * 1: good;
 * -1: invalid pointer;
 * -2: p_q is empty;
 * -3: p_removed == NULL
 */
int SQueue_pop(SQueue * p_q, void ** p_removed){
    int res_fun = 0;
    if(p_q == NULL) return -1;    
    pSQueue_Lock(p_q);
    res_fun = pSQueue_pop(p_q, p_removed);
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief Remove element from p_q and put it in p_removed.
 *        If p_q is empty wait until one elements is added.
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_removed Requirements: p_removed != NULL. This memory location will contains the removed element.
 * @return int: result code:
 * 1: good;
 * -1: invalid pointer;
 * -2: p_q is empty;
 * -3: p_removed == NULL
 */
int SQueue_popWait(SQueue * p_q, void ** p_removed){
    int res_fun = 0;
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);

    while (pSQueue_isEmpty(p_q) == 1) pSQueue_WaitEmpty(p_q);

    if( (res_fun = pSQueue_pop(p_q, p_removed)) == 1){
        pSQueue_WaitFull(p_q);
    } 
    
    pSQueue_Unlock(p_q);
    return res_fun;    
}

static int pSQueue_pop(SQueue * p_q, void ** p_removed){
    int res_fun = 0;
    if(pSQueue_isEmpty(p_q) == 1)//Is empty
        res_fun = -2;
    else{
        //remove from tail
        Node * aux;
        if(p_removed == NULL) return -3;
        aux = p_q->h;
        *p_removed = aux->data;
        p_q->h = p_q->h->next;
        pSQueue_freeNode(aux);
        p_q->n--;
        res_fun = 1;
    }
    return res_fun;
}

/**
 * @brief Check if p_q is empty.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @return int: result code:
 * 0:not empty
 * 1:empty
 * -1: invalid pointer
 */
int SQueue_isEmpty(SQueue * p_q){
    int res_fun = 0;
    if(p_q == NULL) return -1; 
    pSQueue_Lock(p_q);    
    if(p_q == NULL) return -1;
    res_fun = pSQueue_isEmpty(p_q);
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief Check if p_q is full
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @return int: result code:
 * 0:not full
 * 1:full
 * -1: invalid pointer
 */
int SQueue_isFull(SQueue * p_q){
    int res_fun = 0;
    if(p_q == NULL) return -1;    
    pSQueue_Lock(p_q);      
    res_fun = pSQueue_isFull(p_q);
    pSQueue_Unlock(p_q);  
    return res_fun;
}

static int pSQueue_isEmpty(SQueue * p_q){
    return p_q->n == 0 ? 1:0;
}

static int pSQueue_isFull(SQueue * p_q){
    if(p_q->max <= 0) return 0; //if max limit is not set, p_q is never considered full.
    return  p_q->n == p_q->max ? 1:0;
}