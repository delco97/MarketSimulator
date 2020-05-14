/**
 * @file TUser.c
 * @brief  Implementation of User thread.
 */

#include <TUser.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilities.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_UserCounter = 1; /**<  Used to track the number of users created by the application. 
                                    This is used to always generate unique ids for users.*/

//Private functions
static void pUser_Lock(User * p_u) {if(pthread_mutex_lock(&p_u->lock) != 0) err_quit("An error occurred during locking.");}
static void pUser_Unlock(User * p_u) {if(pthread_mutex_unlock(&p_u->lock) != 0) err_quit("An error occurred during unlocking.");}
static int pUser_getNextId() {
    int next = 0;
    if(pthread_mutex_lock(&g_lock) != 0) err_quit("An error occurred during locking.");
    next = g_UserCounter;
    g_UserCounter++;
    if(pthread_mutex_unlock(&g_lock) != 0) err_quit("An error occurred during unlocking.");
    return next;
}

/**
 * @brief 
 * 
 * @param p_products Number of products in the cart.
 * @param p_shoppingTime Time to spend in shopping area.
 * @return User* pointer to new user allocated, NULL if a probelm occurred during allocation. 
 */
User * User_init(int p_products, int p_shoppingTime){
    User * aux = NULL;
    
    if( (aux = malloc(sizeof(User))) != NULL ){
        aux->authTime = 0;
        aux->cashDeskTime = 0;
        aux->id = pUser_getNextId();
        aux->products = p_products;
        aux->queueChanges = 0;
        aux->shoppingTime = p_shoppingTime;
        //Locking system setup
        if (pthread_mutex_init(&(aux->lock), NULL) != 0)  goto err;
    }
    return aux;
err:
    if(aux != NULL) free(aux);
    return NULL;
}

/**
 * @brief Get the p_u id.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int
 */
int getId(User * p_u){ int x; pUser_Lock(p_u); x = p_u->id; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the number of products in cart.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int getProducts(User * p_u){ int x; pUser_Lock(p_u); x = p_u->products; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the number of queue changes occurred.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int getQueueChanges(User * p_u){ int x; pUser_Lock(p_u); x = p_u->queueChanges; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the shopping time to spend in shopping area.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int getShoppingTime(User * p_u){ int x; pUser_Lock(p_u); x = p_u->shoppingTime; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the time spent in a cash desk queue.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int getCashDeskTime(User * p_u){ int x; pUser_Lock(p_u); x = p_u->cashDeskTime; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the time spent before getting the exit authorization.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int getAuthTime(User * p_u){ int x; pUser_Lock(p_u); x = p_u->authTime; pUser_Unlock(p_u); return x; }

/**
 * @brief changeQueue event occurred.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void changeQueue(User * p_u){ pUser_Lock(p_u); p_u->queueChanges++; pUser_Unlock(p_u);}

/**
 * @brief Entry point for a User thread.
 * 
 * Function to use on User thread creation as entry point.  
 * @param p_arg argument passed to the User thread. TUserArg type expected.
 * @return void* 
 */
void * TUser_main(void * p_arg){
    TUserArg * arg = (TUserArg *)p_arg;
    User * u = arg->u;

    printf("[User %d]: start!\n", u->id);
    
    while(1){
        //Shopping time
        printf("[User %d]: start shopping!\n", u->id);
        if(waitMs(u->shoppingTime) == -1)
            err_sys("[User %d]: an error occurred during waiting for shopping time.\n", u->id);
        printf("[User %d]: end shopping!\n", u->id);
        //Handle end of shopping.
        //TODO    

    }


    printf("[User %d]: end\n", u->id);
    return (void *)NULL;
}
