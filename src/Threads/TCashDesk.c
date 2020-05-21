/**
 * @file TCashDesk.c
 * @brief CashDesk implementation.
 * 
 */

#include <TCashDesk.h>
#include <TUser.h>
#include <utilities.h>

//Private functions
static void pDeallocUser(void * p_arg){
	User * u = (User *) p_arg;
	User_delete(u);
}

void CashDesk_Lock(CashDesk * p_c){Lock(&p_c->lock);}
void CashDesk_Unlock(CashDesk * p_c) {Unlock(&p_c->lock);}

/**
 * @brief Create a new CashDesk object.
 * 
 * @param p_m Market in which the CashDesk operate
 * @param p_c Where the new CashDesk will be placed
 * @param p_id id number
 * @param p_serviceConst service costant time for each users served
 * @param p_state starting state
 * @return int: result codes
 * 1: Good init
 */
int CashDesk_init(Market * p_m, CashDesk * p_c, int p_id, int p_serviceConst, CashDeskState p_state) {
    CashDesk aux;
    int isLockInit = 0;

    aux.id = p_id;
    aux.usersPay = NULL;
    aux.serviceConst = p_serviceConst;
    aux.state = p_state;
    aux.market = p_m;

    if((aux.usersPay = SQueue_init(-1)) == NULL) goto err;
	//Init lock system
	if (pthread_mutex_init(&(aux.lock), NULL) != 0) {
		err_msg("An error occurred during locking system initialization. Impossible to setup CashDesk.");
		goto err;
	}
	isLockInit = 1;    

    *p_c = aux;
    return 1;
err:
    if(aux.usersPay != NULL) SQueue_deleteQueue(aux.usersPay, NULL);
    if(isLockInit) pthread_mutex_destroy(&aux.lock);
    return -1;
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

//Getters
CashDeskState CashDesk_getId(CashDesk * p_c) {return p_c->id;}
CashDeskState CashDesk_getSate(CashDesk * p_c) {return p_c->state;}
SQueue * CashDesk_getUsersPay(CashDesk * p_c) {return p_c->usersPay;}
Market * CashDesk_getMarket(CashDesk * p_c) {return p_c->market;}

/**
 * @brief 
 * 
 * @param p_c Requirements: p_c != NULL and must refer to a User object created with #CashDesk_init. Target CashDesk.
 * @param p_u User to add in queue
 */
void CashDesk_addUser(CashDesk * p_c, User * p_u) {
    CashDesk_Lock(p_c);
    if(SQueue_push(p_c->usersPay, p_u) != 1)
        err_quit("Impossible to add user to queue of cash desk %d", p_c->id);
    CashDesk_Unlock(p_c);
}


/**
 * @brief Start CashDesk thread.
 *        The behaviour is undefined if p_u has not been previously initialized with #CashDesk_init.
 * 
 * @param p_u Requirements: p_m != NULL and must refer to a CashDesk object created with #CashDesk_init. Target CashDesk.
 * @return int: result pf pthread_create call
 */
void * CashDesk_main(void * p_arg){
	CashDesk * c = (CashDesk *) p_arg;
    Market * m = CashDesk_getMarket(c);
	User * servedUser = NULL;
    void * data = NULL;

    printf("[CashDesk %d]: start of thread.\n", CashDesk_getId(c));
    
    while (1) {
       	//Wait a signal or new user in desk queue to proceed
		Lock(&m->lock);
		while (sig_hup != 1 && sig_quit != 1 && SQueue_isEmpty(c->usersPay)==1) 
			pthread_cond_wait(&m->cv_MarketNews, &m->lock);
		Unlock(&m->lock);
		if(sig_hup == 1 || sig_quit == 1) {
            //Empties the user desk queue and wait until no other users are in the market
            while (Market_isEmpty(m)!=1) {
                if(SQueue_pop(c->usersPay, &data) == 1) {
                    servedUser = (User *)data;
                    User_setStartPaymentTime(servedUser, getCurrentTime());
                    //printf("[CashDesk %d]: started to serve user %d.\n", CashDesk_getId(c), User_getId(servedUser));
                    if(sig_hup == 1) {//Serve users only if it is a slow closing
                        if(waitMs(c->serviceConst + User_getProducts(servedUser) * Market_getNP(m)) == -1)
                            err_sys("[User %d]: an error occurred during waiting for shopping time.\n", User_getId(servedUser));
                    }
                    //printf("[CashDesk %d]: user served %d.\n", CashDesk_getId(c), User_getId(servedUser));
                    Market_moveToExit(m, servedUser);
                }
            }
            break;
        }
        //Market is not closing
        if(SQueue_pop(c->usersPay, &data) == 1){
            servedUser = (User *)data;
            User_setStartPaymentTime(servedUser, getCurrentTime());
            //printf("[CashDesk %d]: started to serve user %d.\n", CashDesk_getId(c), User_getId(servedUser));
            if(waitMs(c->serviceConst + User_getProducts(servedUser) * Market_getNP(m)) == -1)
                err_sys("[User %d]: an error occurred during waiting for shopping time.\n", User_getId(servedUser));
            //printf("[CashDesk %d]: user served %d.\n", CashDesk_getId(c), User_getId(servedUser));
            Market_moveToExit(m, servedUser);
        }
    }
    
	printf("[CashDesk %d]: end of thread.\n", CashDesk_getId(c));
    return (void *)NULL;
}