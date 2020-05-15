/**
 * @file TUser.c
 * @brief  Implementation of User thread.
 */

#include <TUser.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilities.h>
#include <pthread.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_UserCounter = 1; /**<  Used to track the number of users created by the application. 
                                    This is used to always generate unique ids for users.*/

//Private functions
static void pUser_Lock(User * p_u) {Lock(&p_u->lock);}
static void pUser_Unlock(User * p_u) {Unlock(&p_u->lock);}
static void pUser_WaitExit(User * p_u) {Wait(&p_u->cv_out, &p_u->lock);}
static void pUser_SignalExit(User * p_u) {Signal(&p_u->cv_out);}
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
 * @param p_m Reference to Market where the user is.
 * @return User* pointer to new user allocated, NULL if a probelm occurred during allocation. 
 */
User * User_init(int p_products, int p_shoppingTime, Market * p_m){
    User * aux = NULL;
    
    if( (aux = malloc(sizeof(User))) != NULL ){
        aux->id = pUser_getNextId();
        aux->products = p_products;
        aux->queueChanges = 0;
        aux->state = USR_SHOPPING;
        aux->shoppingTime = p_shoppingTime;
        aux->market = p_m;
        //Locking system setup
        if (pthread_mutex_init(&(aux->lock), NULL) != 0 ||
            pthread_cond_init(&(aux->cv_out), NULL) != 0)  goto err;
    }
    return aux;
err:
    if(aux != NULL) free(aux);
    return NULL;
}

/**
 * @brief Dealloc a User object.
 * 
 * @warning This function should be called by only one thread when no other thread is working on p_u object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #SQueue_init. Target User.
 * @return int: result code:
 *  1: p_u != NULL and the deallocation proceed witout errors. 
 *  -1: p_u == NULL
 */
int User_delete(User * p_u) {
    if(p_u == NULL) return -1; 
    pthread_mutex_destroy(&p_u->lock);
    pthread_cond_destroy(&p_u->cv_out);
    free(p_u);
    return 1;
}

/**
 * @brief Start User thread.
 *        The behaviour is undefined if p_u has not been previously initialized with #User_init.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int: result pf pthread_create call
 */
int User_startThread(User * p_u) {
    return pthread_create(&p_u->thread, NULL, User_main, p_u);
}

/**
 * @brief Join the user thread
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int result of pthread_join
 */
int User_joinThread(User * p_u){
    return pthread_join(p_u->thread, NULL);
}

/**
 * @brief Get the p_u id.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int
 */
int User_getId(User * p_u){ int x; pUser_Lock(p_u); x = p_u->id; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the number of products in cart.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int User_getProducts(User * p_u){ int x; pUser_Lock(p_u); x = p_u->products; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the number of queue changes occurred.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int User_getQueueChanges(User * p_u){ int x; pUser_Lock(p_u); x = p_u->queueChanges; pUser_Unlock(p_u); return x; }

/**
 * @brief Get current user state
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return UserState 
 */
UserState User_getState(User * p_u) {UserState x; pUser_Lock(p_u); x = p_u->state; pUser_Unlock(p_u); return x; }

/**
 * @brief Get entry time in the market
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return struct timespec 
 */
struct timespec User_getMarketEntryTime(User * p_u)
{ struct timespec x; pUser_Lock(p_u); x = p_u->tMarketEntry; pUser_Unlock(p_u); return x; }

/**
 * @brief Get exit time in the market
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return struct timespec 
 */
struct timespec User_getMarketExitTime(User * p_u)
{ struct timespec x; pUser_Lock(p_u); x = p_u->tMarketExit; pUser_Unlock(p_u); return x; }

/**
 * @brief Get entry time when user moved to a queue.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return struct timespec 
 */
struct timespec User_getQueueStartTime(User * p_u)
{ struct timespec x; pUser_Lock(p_u); x = p_u->tQueueStart; pUser_Unlock(p_u); return x; }

/**
 * @brief Get time when a cashier started to serve it
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return struct timespec 
 */
struct timespec User_getStartPaymentTime(User * p_u)
{ struct timespec x; pUser_Lock(p_u); x = p_u->tStartPayment; pUser_Unlock(p_u); return x; }

/**
 * @brief Get the shopping time to spend in shopping area.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int User_getShoppingTime(User * p_u){ int x; pUser_Lock(p_u); x = p_u->shoppingTime; pUser_Unlock(p_u); return x; }

/**
 * @brief Set current state of p_u
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_x 
 */
void User_setState(User * p_u, UserState p_x){
    pUser_Lock(p_u); p_u->state = p_x; pUser_Unlock(p_u);
}

/**
 * @brief Set time of when p_u entered the market.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_x time of enter
 */
void User_setMarketEntryTime(User * p_u, struct timespec p_x){
    pUser_Lock(p_u); p_u->tMarketEntry = p_x; pUser_Unlock(p_u);
}

/**
 * @brief Set time of when p_u exit from the market.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_x time of exit
 */
void User_setMarketExitTime(User * p_u, struct timespec p_x){
    pUser_Lock(p_u); p_u->tMarketExit = p_x; pUser_Unlock(p_u);
}
/**
 * @brief Set time of when p_u moved to a queue.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_x time of when user moved in front of a queue.
 */
void User_setQueueStartTime(User * p_u, struct timespec p_x){
    pUser_Lock(p_u); p_u->tQueueStart = p_x; pUser_Unlock(p_u);
}
/**
 * @brief Set time of when a cashier started to process p_u products.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_x time of when a cashier started to process p_u products.
 */
void User_setStartPaymentTime(User * p_u, struct timespec p_x){
    pUser_Lock(p_u); p_u->tStartPayment = p_x; pUser_Unlock(p_u);
}

/**
 * @brief changeQueue event occurred.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void User_changeQueue(User * p_u){ pUser_Lock(p_u); p_u->queueChanges++; pUser_Unlock(p_u);}


/**
 * @brief Send signal to the user to notify user exit.
 * 
 * @param p_u 
 */
void User_signalExit(User * p_u){
    pUser_SignalExit(p_u);
}

/**
 * @brief Wait until p_u is ou of the market.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void User_waitExit(User * p_u) {
    pUser_Lock(p_u);
    
    while(p_u->state != USR_OUT) pUser_WaitExit(p_u);
    
    pUser_Unlock(p_u);
}

/**
 * @brief Entry point for a User thread.
 * 
 * Function to use on User thread creation as entry point.  
 * @param p_arg argument passed to the User thread. TUserArg type expected.
 * @return void* 
 */
void * User_main(void * p_arg){
    User * u = (User *)p_arg;

    User_setMarketEntryTime(u, getCurrentTime());
    printf("[User %d]: main thread start!\n", u->id);
    
    //Shopping time
    printf("[User %d]: start shopping!\n", u->id);
    if(waitMs(u->shoppingTime) == -1)
        err_sys("[User %d]: an error occurred during waiting for shopping time.\n", u->id);
    printf("[User %d]: end shopping!\n", u->id);
    //End of shopping, move to one cashdesk
    if(u->products > 0){//Has something in the cart
        printf("[User %d]: move to a open cash desk for payment.\n", u->id);
        //TODO: User_moveToPay(u)
    }else{//Nothing in the cart
        printf("[User %d]: move to the authorization queue.\n", u->id);
        //Move User struct to queue of users waiting director authorization before exit.
        //TODO: User_moveToAuth(u)
    }
    
    User_waitExit(u);//Wait until user exit
    
    printf("[User %d]: exit from the market.\n", u->id);
    return (void *)NULL;
}
