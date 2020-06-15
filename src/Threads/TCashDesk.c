/**
 * @file TCashDesk.c
 * @brief CashDesk implementation.
 * 
 */

#include <TCashDesk.h>
#include <TUser.h>
#include <utilities.h>
#include <Config.h>
#include <stdlib.h>

#define MAX_DESK_STR 2048 /**< Max length of a string used to rapresent a CashDesk object*/

//Private functions
static void pDeallocUser(void * p_arg){
	User * u = (User *) p_arg;
	User_delete(u);
}
static void pCashDesk_toString(CashDesk * p_c, char * p_buff){
    sprintf(p_buff, "[CashDesk %d]: products=%d clients=%d open_time=%.3f avg_service_time=%.3f closures=%d\n", 
            p_c->id,
            p_c->productsProcessed, 
            p_c->usersProcessed,
            (double)(p_c->totOpenTime > 0 ? (double)p_c->totOpenTime/1000:0),
            (double)(p_c->avgServiceTime > 0 ? (double)p_c->avgServiceTime/1000:0),
            p_c->numClosure);
}

void CashDesk_Lock(CashDesk * p_c){Lock(&p_c->lock);}
void CashDesk_Unlock(CashDesk * p_c) {Unlock(&p_c->lock);}

/**
 * @brief Create a new CashDesk object.
 * 
 * @param p_m Market in which the CashDesk operate
 * @param p_id id number
 * @param p_serviceConst service costant time for each users served
 * @param p_state starting state
 * @param p_notifyInterval notify interval to inform director thread in ms
 * @return int: result codes
 * 1: Good init
 */
CashDesk * CashDesk_init(Market * p_m, int p_id, int p_serviceConst, int p_notifyInterval, CashDeskState p_state) {
    CashDesk * aux = NULL;
    int isLockInit = 0;

    if((aux = malloc(sizeof(CashDesk))) == NULL) {
		ERR_MSG("An error occurred during cash desk creation. ");
		goto err;
	}

    aux->id = p_id;
    aux->usersPay = NULL;
    aux->serviceConst = p_serviceConst;
    aux->state = p_state;
    aux->market = p_m;
    aux->productsProcessed = 0;
    aux->usersProcessed = 0;
    aux->numClosure = 0;
    aux->totOpenTime = 0;
    aux->avgServiceTime = 0;
    aux->notifyInterval = p_notifyInterval;

    if((aux->usersPay = SQueue_init(-1)) == NULL) {
        ERR_MSG("An error occurred during creation of queue. Impossible to setup CashDesk.");
        goto err;
    }
	//Init lock system
	if (pthread_mutex_init(&(aux->lock), NULL) != 0 ||
        pthread_cond_init(&aux->cv_DeskNews, NULL) != 0) {
		ERR_MSG("An error occurred during locking system initialization. Impossible to setup CashDesk.");
		goto err;
	}
	isLockInit = 1;    

    return aux;
err:
    if(aux != NULL) {
        if(aux->usersPay != NULL) SQueue_deleteQueue(aux->usersPay, NULL);
        if(isLockInit) {
            pthread_mutex_destroy(&aux->lock);
            pthread_cond_destroy(&aux->cv_DeskNews);
        }
        free(aux);
    }
    return NULL;
}

/**
 * @brief Dealloc a CashDesk object.
 * 
 * @warning This function should be called by only one thread when no other thread is working on p_c object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_c Requirements: p_c != NULL and must refer to a CashDesk object created with #CashDesk_init. Target CashDesk.
 * @return int: result code:
 *  1: p_c != NULL and the deallocation proceed witout errors. 
 *  -1: p_c == NULL
 */
int CashDesk_delete(CashDesk * p_c){
    if(p_c == NULL) return -1;
    SQueue_deleteQueue(p_c->usersPay, pDeallocUser);
    pthread_mutex_destroy(&p_c->lock);
    pthread_cond_destroy(&p_c->cv_DeskNews);
    free(p_c);
    return 1;
}

/**
 * @brief Start CashDesk thread.
 *        The behaviour is undefined if p_u has not been previously initialized with #CashDesk_init.
 * 
 * @param p_u Requirements: p_d != NULL and must refer to a CashDesk object created with #CashDesk_init. Target CashDesk.
 * @return int: result pf pthread_create call
 */
int CashDesk_startThread(CashDesk * p_d){
	return pthread_create(&p_d->thread, NULL, CashDesk_main, p_d);
}

/**
 * @brief Join the CashDesk thread
 * 
 * @param p_c Requirements: p_c != NULL and must refer to a User object created with #CashDesk_init. Target CashDesk.
 * @return int result of pthread_join
 */
int CashDesk_joinThread(CashDesk * p_c){
	return pthread_join(p_c->thread, NULL);
}

/**
 * @brief Add user to queue.
 * 
 * @param p_c Requirements: p_c != NULL and must refer to a User object created with #CashDesk_init. Target CashDesk.
 * @param p_u User to add in queue
 */
void CashDesk_addUser(CashDesk * p_c, User * p_u) {
    if(SQueue_push(p_c->usersPay, p_u) != 1)
        ERR_QUIT("Impossible to add user to queue of cash desk %d", p_c->id);
    Signal(&p_c->market->cv_MarketNews);
}

void CashDesk_log(CashDesk * p_c) {
    CashDesk_Lock(p_c);
    char aux[MAX_DESK_STR];
    pCashDesk_toString(p_c, aux);
    CashDesk_Unlock(p_c);
    Market_log(p_c->market, aux);
}

/**
 * @brief Subthread used to periodically notify the director thread about current desk status.
 * 
 * @param p_arg is expected as CashDesk * object.
 * @return void *
 */
void * CashDesk_notifyDirector(void * p_arg) {
    CashDesk * c = (CashDesk *) p_arg;
    Market * m = c->market;
    Director * d = m->director;
    CashDeskNotify * msg = NULL;
    while (1) {
        if(sig_hup || sig_quit) break;
        if(waitMs(c->notifyInterval) == -1)
            ERR_SYS_QUIT("[User %d]: an error occurred during waiting to notify director thread.\n", c->id);
        //Prepare info for director thread
        msg = NULL;
        if((msg = malloc(sizeof(CashDeskNotify))) == NULL)
            ERR_QUIT("An error occurred during malloc for notify message allocation.");
        msg->id = c->id;
        msg->state = c->state;
        msg->users = SQueue_dim(c->usersPay);
        //Send info to director thread
        SQueue_push(d->notifications, msg);
        Signal(&d->cv_Director_DesksNews);
    }
    return (void *)NULL;
}

/**
 * @brief CashDesk thread main function.
 *        The behaviour is undefined if p_u has not been previously initialized with #CashDesk_init.
 * 
 * @param p_arg is expected as CashDesk * object.
 */
void * CashDesk_main(void * p_arg){
	CashDesk * c = (CashDesk *) p_arg;
    Market * m = c->market;
	User * servedUser = NULL;
    void * data = NULL;
    CashDeskState lastState = c->state;
    CashDeskState currentState = lastState;
    struct timespec lastOpenTime = getCurrentTime();
    pthread_t thNotifyHandler;

    lastState = c->state;
    currentState = lastState;
    lastOpenTime = getCurrentTime();

    //Create sub thread used to notify director thread
    if(pthread_create(&thNotifyHandler, NULL, CashDesk_notifyDirector, c) !=0)
        ERR_QUIT("[Director]: an error occurred during creation of notify thread."); 

    printf("[CashDesk %d]: start of thread.\n", c->id);
    
    while (1) {
       	//Wait a closure signal or new user in desk queue to proceed
		Lock(&c->lock);
		while ( sig_hup != 1 && sig_quit != 1 && SQueue_isEmpty(c->usersPay)==1 && 
                (currentState = c->state) == lastState) 
			pthread_cond_wait(&c->cv_DeskNews, &c->lock);
        Unlock(&c->lock);
       
		if(sig_hup == 1 || sig_quit == 1) {
            //Empties the user desk queue and wait until no other users are in the market
            while (SQueue_isEmpty(c->usersPay) != 1 || SQueue_isEmpty(c->market->usersShopping) != 1) {
                if(SQueue_pop(c->usersPay, &data) == 1) {
                    servedUser = (User *)data;
                    
                    if(sig_hup == 1 && c->state == DESK_OPEN) {//Serve users only if it is a slow closing and cash dek is open
                        printf("[CashDesk %d]: started to serve user %d (time required: %ld).\n", c->id, servedUser->id, c->serviceConst + servedUser->products * m->NP);
                        c->usersProcessed++;
                        c->productsProcessed+=servedUser->products;            
                        c->avgServiceTime += c->serviceConst + servedUser->products * m->NP;
                        if(waitMs(c->serviceConst + servedUser->products * m->NP) == -1)
                            ERR_SYS_QUIT("[User %d]: an error occurred during waiting for shopping time.\n", servedUser->id);
                        printf("[CashDesk %d]: user served %d.\n", c->id, servedUser->id);

                    }else {
                        printf("[CashDesk %d]: user %d exit without paying.\n", c->id, servedUser->id);
                    }
                    
                    Market_moveToExit(m, servedUser);
                }
            }
            if(c->state == DESK_OPEN)
                c->totOpenTime += elapsedTime(lastOpenTime, getCurrentTime());
            
            c->avgServiceTime=c->avgServiceTime/c->usersProcessed;
            break;
        }
        //Market is not closing
        if(currentState != lastState) {//Desk state change
            lastState = currentState;
            printf("[CashDesk %d]:  now is %s.\n", c->id, currentState==DESK_OPEN ? "OPEN":"CLOSE");
            if(currentState == DESK_OPEN){
                lastOpenTime = getCurrentTime();
            } else{//DESK_CLOSE
                c->totOpenTime += elapsedTime(lastOpenTime, getCurrentTime());
                c->numClosure++;
            }
        }        
        if(c->state == DESK_OPEN) {
            if(SQueue_pop(c->usersPay, &data) == 1) {
                servedUser = (User *)data;
                printf("[CashDesk %d]: started to serve user %d (time required: %ld).\n", c->id, servedUser->id, c->serviceConst + servedUser->products * m->NP);
                c->usersProcessed++;
                c->productsProcessed+=servedUser->products;       
                c->avgServiceTime += c->serviceConst + servedUser->products * m->NP;               
                if(waitMs(c->serviceConst + servedUser->products * m->NP) == -1)
                    ERR_SYS_QUIT("[User %d]: an error occurred during waiting for shopping time.\n", servedUser->id);
                printf("[CashDesk %d]: user %d served.\n", c->id, servedUser->id);  
                Market_moveToExit(m, servedUser);
            }
        }
    }
    if(pthread_join(thNotifyHandler, NULL) !=0)
        ERR_QUIT("[Director]: an error occurred during join of notify thread."); 

	printf("[CashDesk %d]: end of thread.\n", c->id);
    return (void *)NULL;
}