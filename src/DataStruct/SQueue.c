/**
 * @file SQueue.c
 * @brief   A SQueue is a mutable thread safe queue in which generic elements (void *) can be added or removed.
 *          A typical SQueue (abstract rappresentation):
 *              - <x1,x2,...,xn>: elements in queue
 *              - n: number of element currently in the queue
 *              - x1: is the tail element
 *              - xn: is the head element
 *              - max: maximum number of elements allowed in the queue (<=0: means no limit)
 */

#include <utilities.h>
#include <SQueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//Private functions prototypes
static Node * pSQueue_allocateNode() { return malloc(sizeof(Node));}
static SQueue * pSQueue_allocQueue() { return malloc(sizeof(SQueue)); }
static void pSQueue_freeNode(Node * n, funDealloc p_funDealloc) { 
    if(p_funDealloc != NULL) p_funDealloc(n->data); 
    free(n); 
}
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
    
    aux = pSQueue_allocQueue();
    if(aux != NULL){
        aux->max = p_max;
        aux->n = 0;
        aux->h = NULL;  
        aux->t = NULL;
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
 * @param p_f Function to use for deallocate data nodes. If NULL, no deallocation.
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @return int: result code:
 *  1: p_q != NULL and the deallocation proceed witout errors. 
 *  -1: p_q == NULL
 */
int SQueue_deleteQueue(SQueue * p_q, funDealloc p_f){
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);
    Node * aux = NULL;
    while (p_q->h != NULL) {
        aux = p_q->h;
        p_q->h= p_q->h->next;
        //Deallocate Node
        pSQueue_freeNode(aux, p_f);
    }
    pSQueue_Unlock(p_q);
    pthread_mutex_destroy(&p_q->lock);
    pthread_cond_destroy(&p_q->cv_empty);
    pthread_cond_destroy(&p_q->cv_full);
    free(p_q);
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
    if( (res_fun = pSQueue_push(p_q, p_new)) == 1 )
        pSQueue_SignalEmpty(p_q);
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief   Add a new element p_new in queue p_q only if the queue is not full.
 *          If queue is full, it waits until there is space to add the new element.
 *          Note: if the queue is set as unlimited queue will never be full.
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
        //Init new node
        Node * aux = pSQueue_allocateNode();
        if(aux == NULL) return -3;
        aux->data = p_new;
        aux->next = NULL;
        //Add new node in tail
        if(p_q->t == NULL){//Empty queue
            p_q->h = aux;
            p_q->t = aux;
        }else{//No empty queue, add in tail.
            p_q->t->next = aux;
            p_q->t = aux;
        }
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
    if( (res_fun = pSQueue_pop(p_q, p_removed)) == 1)
        pSQueue_SignalFull(p_q);
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
 * -3: p_removed == NULL
 */
int SQueue_popWait(SQueue * p_q, void ** p_removed){
    int res_fun = 0;
    if(p_q == NULL) return -1;
    if(p_removed == NULL) return -3;
    pSQueue_Lock(p_q);

    while (pSQueue_isEmpty(p_q) == 1) pSQueue_WaitEmpty(p_q);
    
    if( (res_fun = pSQueue_pop(p_q, p_removed)) == 1){
        pSQueue_SignalFull(p_q);
    } 
    
    pSQueue_Unlock(p_q);
    return res_fun;    
}

static int pSQueue_pop(SQueue * p_q, void ** p_removed){
    int res_fun = 0;
    if(p_removed == NULL) return -3;
    if(pSQueue_isEmpty(p_q) == 1)//Is empty
        res_fun = -2;
    else{
        //remove from tail
        Node * aux;
        aux = p_q->h;
        *p_removed = aux->data;
        if(p_q->h == p_q->t){//One element left
           p_q->h = NULL;
           p_q->t = NULL; 
        }else{//More then one element
            p_q->h = p_q->h->next;
        }
        //Free memory allocated only for Node struct.
        //Data contained inside node is not deallocated.
        pSQueue_freeNode(aux, NULL);
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

/**
 * @brief Print data contained in all nodes of p_q.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_funPrint Requirements: p_funPrint != NULL. Function used tu print nodes data.
 */
void SQueue_print(SQueue * p_q, funPrint p_funPrint){
    pSQueue_Lock(p_q);
    fprintf(stdout, "Queue Length: %ld\n", p_q->n);
    fprintf(stdout, "Elements:\n");
    Node * aux = p_q->h;
    char buf[MAX_STRING_NODE];
    while(aux != NULL) {
        p_funPrint(buf, MAX_STRING_NODE, aux->data);
        fprintf(stdout, "%s   ", buf);
        aux = aux->next;
    }
    fprintf(stdout, "\n");

    pSQueue_Unlock(p_q);
}

/**
 * @brief Get number of elements currently inside p_q
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @return int: result code:
 * -1: invalid pointer
 * [0;n]: number of elements inside p_q
 */
int SQueue_dim(SQueue * p_q){
    int res_fun = -1;
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);
    res_fun = p_q->n;
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief Find p_target inside p_q using p_funCmp as comapre criteria.
 * If found return its position inside list.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_target data to search
 * @param p_funCmp function used for compare data of nodes (result value as strcmp).
 * @return int: result code:
 * [0;n]: first node with p_target data.
 * -1: invalid pointer;
 * -2: invalid compare function
 * -3: p_target not found
 */
int SQueue_find(SQueue * p_q, void * p_target, funCmp p_funCmp){
    if(p_q == NULL) return -1;
    if(p_funCmp == NULL) return -2;
    pSQueue_Lock(p_q);
    int i=0;
    int res_fun = -3;
    Node * cur = p_q->h;
    while (cur != NULL) {
        if(p_funCmp(p_target, cur->data) == 0) {
            res_fun = i;
            break;
        }
        cur = cur->next;
        i++;
    }
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief   Find first occurrence of p_target inside p_q using p_funCmp as comapre criteria
 *          and remove it from the queue if it's found.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_target data to remove
 * @param p_funCmp function used for compare data of nodes (result value as strcmp).
 * @return int: result code:
 *  1: p_target found and removed
 * -1: invalid pointer;
 * -2: invalid compare function
 * -3: p_target not found
 */
int SQueue_remove(SQueue * p_q, void * p_target, funCmp p_funCmp){
    if(p_q == NULL) return -1;
    if(p_funCmp == NULL) return -2;    
    pSQueue_Lock(p_q);
    int res_fun = -3;
    Node * prev = NULL;
    Node * cur = p_q->h;
    Node * aux;
    if(p_q->n > 0){//There is at least one element
        //Search target to remove

        while (cur != NULL) {
            if(p_funCmp(p_target, cur->data) == 0){//Check if found
                res_fun = 1;
                break;
            }
            prev = cur;
            cur = cur->next;
        }
        if(cur != NULL){//Target found
            p_q->n--;
            if(p_q->h == p_q->t){//Only one element in queue
                pSQueue_freeNode(cur, NULL);
                p_q->h = NULL;
                p_q->t = NULL;
            }else{//At least two elements in queue
                if(cur == p_q->h){//Target in head
                    aux = cur->next;
                    pSQueue_freeNode(cur, NULL);
                    p_q->h = aux;
                }else if(cur == p_q->t){//Target in tail
                    pSQueue_freeNode(cur, NULL);
                    p_q->t = prev;
                    p_q->t->next = NULL;
                }else{//Target in the middle of the queue (at least 3 elements)
                    prev->next = cur->next;
                    pSQueue_freeNode(cur, NULL);
                }
            }
        }
    }
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief   Remove item from position p_pos.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_pos position of the node to remove.
 * @param p_removed will contain data removed if an element in position p_pos exists
 * @return int: result code:
 *  1: element in position p_pos exist and removed
 * -1: invalid pointer;
 * -2: p_q is empty
 * -3: invalid p_pos
 */
int SQueue_removePos(SQueue * p_q, int p_pos, void ** p_removed) {
    if(p_q == NULL) return -1;
    pSQueue_Lock(p_q);
    if(p_q->n == 0) return -2;
    if(p_pos < 0 || p_pos >= p_q->n) return -3;    
    int res_fun = -3;
    int i = 0;
    Node * prev = NULL;
    Node * cur = p_q->h;
    Node * aux;
    if(p_q->n > 0) {//There is at least one element
        res_fun = 1;
        //Search target to remove
        while (cur != NULL) {
            if(i == p_pos) break;
            prev = cur;
            cur = cur->next;
            i++;
        }
        //Element found for sure
        assert(cur != NULL);
        *p_removed = cur->data;
        p_q->n--;
        if(p_q->h == p_q->t){//Only one element in queue
            pSQueue_freeNode(cur, NULL);
            p_q->h = NULL;
            p_q->t = NULL;
        }else{//At least two elements in queue
            if(cur == p_q->h){//Target in head
                aux = cur->next;
                pSQueue_freeNode(cur, NULL);
                p_q->h = aux;
            }else if(cur == p_q->t){//Target in tail
                pSQueue_freeNode(cur, NULL);
                p_q->t = prev;
                p_q->t->next = NULL;
            }else{//Target in the middle of the queue (at least 3 elements)
                prev->next = cur->next;
                pSQueue_freeNode(cur, NULL);
            }
        }

    }
    pSQueue_Unlock(p_q);
    return res_fun;
}

/**
 * @brief Apply function p_funMap over data contained in each node.
 * 
 * @param p_q Requirements: p_q != NULL and must refer to a SQueue object created with #SQueue_init. Target SQueue.
 * @param p_funMap function to applay
 */
void SQueue_map(SQueue * p_q, funMap p_funMap) {
    pSQueue_Lock(p_q);
    Node * aux = p_q->h;
    while(aux != NULL) {
        p_funMap(aux->data);
        aux = aux->next;
    }
    pSQueue_Unlock(p_q);  
}



static int pSQueue_isEmpty(SQueue * p_q){
    return p_q->n == 0 ? 1:0;
}

static int pSQueue_isFull(SQueue * p_q){
    if(p_q->max <= 0) return 0; //if max limit is not set, p_q is never considered full.
    return  p_q->n == p_q->max ? 1:0;
}