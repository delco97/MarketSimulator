/**
 * @file TUser.c
 * @brief  Implementation of User.
 */

#include <TUser.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilities.h>
#include <pthread.h>

#define MAX_USR_STR 2048

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_UserCounter = 1; /**<  Used to track the number of users created by the application. 
                                    This is used to always generate unique ids for users.*/

//Private functions
static void pUser_Lock(User * p_u) {Lock(&p_u->lock);}
static void pUser_Unlock(User * p_u) {Unlock(&p_u->lock);}
static int pUser_getNextId() {
    int next = 0;
    if(pthread_mutex_lock(&g_lock) != 0) err_quit("An error occurred during locking.");
    next = g_UserCounter;
    g_UserCounter++;
    if(pthread_mutex_unlock(&g_lock) != 0) err_quit("An error occurred during unlocking.");
    return next;
}
static void pUser_toString(User * p_u, char * p_buff){
    double marketTime = (double)elapsedTime(p_u->tMarketEntry, p_u->tMarketExit);
    double queueTime = (double)elapsedTime(p_u->tQueueStart, p_u->tMarketExit);
    sprintf(p_buff, "[User %d]: products=%d tot_time_market=%.3f tot_time_queue=%.3f queue_visited=%d\n", 
            p_u->id,
            p_u->products, 
            (double)(marketTime > 0 ? marketTime/1000:0),
            (double)(queueTime > 0 ? queueTime/1000:0),
            p_u->queueChanges);
}

/**
 * @brief Create a new User object.
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
        aux->state = USR_READY;
        aux->products = p_products;
        aux->queueChanges = 0;
        aux->shoppingTime = p_shoppingTime;
        aux->market = p_m;
        //Locking system setup
        if (pthread_mutex_init(&(aux->lock), NULL) != 0 ||
            pthread_cond_init(&aux->cv_UserNews, NULL) != 0)  goto err;
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
    pthread_cond_destroy(&p_u->cv_UserNews);
    free(p_u);
    return 1;
}

/**
 * @brief   Reset the p_u structure for a next reuse.
 * 
 * @warning No other thread should be using p_u when thi function is called and the p_u should not be running.        
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_products Number of products in the cart.
 * @param p_shoppingTime Time to spend in shopping area.
 * @param p_m Reference to Market where the user is.
 */
void User_reset(User * p_u, int p_products, int p_shoppingTime, Market * p_m){
    pUser_Lock(p_u);
    p_u->id = pUser_getNextId();
    p_u->products = p_products;
    p_u->queueChanges = 0;
    p_u->shoppingTime = p_shoppingTime;
    p_u->market = p_m;
    pUser_Unlock(p_u);
}

/**
 * @brief Log user info.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void User_log(User * p_u){
    pUser_Lock(p_u);
    char aux[MAX_USR_STR];
    pUser_toString(p_u, aux);
    pUser_Unlock(p_u);
    Market_log(p_u->market, aux);
}

/**
 * @brief Comapre two user object by subtracting their ids.
 * @param p_1 Requirements: p_u != NULL and must refer to a User object created with #User_init.
 * @param p_2 Requirements: p_u != NULL and must refer to a User object created with #User_init.
 * @return int: result code
 * <0: p_u1 has a littler id then p_u2
 * >0: p_u1 has a greater id then p_u2
 * =0: p_u1 and p_u2 have the same id
 */
int User_compare(void * p_1, void * p_2){
    return ((User *) p_1)->id - ((User *) p_2)->id;
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
 * @brief Get current user state
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return UserState 
 */
UserState User_getState(User * p_u) { UserState x; pUser_Lock(p_u); x = p_u->state; pUser_Unlock(p_u); return x; }


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
 * @brief Get the shopping time to spend in shopping area.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return int 
 */
int User_getShoppingTime(User * p_u){ int x; pUser_Lock(p_u); x = p_u->shoppingTime; pUser_Unlock(p_u); return x; }

/**
 * @brief Set user state
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void User_setState(User * p_u, UserState p_s) {pUser_Lock(p_u); p_u->state = p_s; pUser_Unlock(p_u);}


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
 * @brief Set number of products purchases
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @param p_prd new number of products
 */
void User_setProducts(User * p_u, int p_prd) {
    pUser_Lock(p_u); p_u->products = p_prd; pUser_Unlock(p_u);
}

/**
 * @brief Get the market where user p_u is located.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 * @return Market* 
 */
Market * User_getMarket(User * p_u) {return p_u->market;}


/**
 * @brief changeQueue event occurred.
 * 
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void User_changeQueue(User * p_u){ pUser_Lock(p_u); p_u->queueChanges++; pUser_Unlock(p_u);}

/**
 * @brief Entry point for a User thread.
 * 
 * Function to use on User thread creation. This
 * function handle the User data structure passed as argument in the following way:
 *  1. Set user entry time in the market.
 *  2. Wait for simulate shopping
 *  3. Move the user in to one open cash desk if he has at least one product, otherwise
 *     he is moved to authorization queue.
 *  During these three consecutive steps is checked if a fast closing signal has been rised (sig_quit ==1).
 *  In that case the user is moved directly to exit queue.
 * @param p_arg argument passed to the User thread. User type expected.
 * @return void* 
 */
void * User_main(void * p_arg) {
    User * u = (User *)p_arg;
    Market * m = User_getMarket(u);
    while (1) {
        Lock(&u->lock);
        //Wait to being ready to start next simulation
        while (u->state == USR_NOT_READY)
            pthread_cond_wait(&u->cv_UserNews, &u->lock);
        Unlock(&u->lock);

        if(User_getState(u) == USR_QUIT) break;

        User_setState(u, USR_NOT_READY);
        //USR_READY => Is in shopping area ready to start simulation
        User_setMarketEntryTime(u, getCurrentTime());
        if(sig_quit) {
            Market_FromShoppingToExit(m, u);
            break;
        }

        //Shopping time
        printf("[User %d]: start shopping!\n", User_getId(u));
        
        if(waitMs(User_getShoppingTime(u)) == -1)
            err_sys("[User %d]: an error occurred during waiting for shopping time.\n", User_getId(u));

        if(sig_quit == 1) {
            Market_FromShoppingToExit(User_getMarket(u), u);
            break;
        }
        printf("[User %d]: end shopping!\n", User_getId(u));
        //End of shopping, move to one cashdesk or to authorization queue
        if(User_getProducts(u) > 0){//Has something in the cart
            printf("[User %d]: move to a open cash desk for payment.\n", User_getId(u));
            Market_FromShoppingToPay(User_getMarket(u), u);
        }else{//Nothing in the cart
            printf("[User %d]: move to the authorization queue.\n", User_getId(u));
            //Move User struct to queue of users waiting director authorization before exit.
            Market_FromShoppingToAuth(User_getMarket(u), u);
        }
    }
    printf("[User %d]: end of thread.\n", User_getId(u));
    return (void *)NULL;
}
