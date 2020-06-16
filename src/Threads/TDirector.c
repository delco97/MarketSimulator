/**
 * @file TDirector.c
 * @brief Director implementation.
 * 
 */

#include <TDirector.h>
#include <TMarket.h>
#include <TMarket.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilities.h>
#include <TCashDesk.h>

void Director_Lock(Director * p_d) {Lock(&p_d->lock);}
void Director_Unlock(Director * p_d) {Unlock(&p_d->lock);}

/**
 * @brief Create a new Director object.
 * 
 * @param p_m Market in which the director operate
 * @return Market* pointer to new market allocated, NULL if a probelm occurred during allocation. 
 */
Director * Director_init(Market * p_m) {
    Director * aux = NULL;
    
    if( (aux = malloc(sizeof(Director))) == NULL )
		goto err;
	
    aux->market = p_m;
    aux->notifications = NULL;

    if((aux->notifications = SQueue_init(-1)) == NULL) {
		ERR_MSG("An error occurred during notification queue setup. Impossible to setup the director.");
        goto err;
	}

	if (pthread_cond_init(&aux->cv_Director_AuthNews, NULL) != 0 ||
        pthread_cond_init(&aux->cv_Director_DesksNews, NULL) != 0 ||
        pthread_mutex_init(&(aux->lock), NULL) != 0) {
		ERR_MSG("An error occurred during locking system initialization. Impossible to setup the director.");
        goto err;
	}

	
    return aux;
err:
    if(aux != NULL) free(aux);
    if(aux->notifications != NULL) SQueue_deleteQueue(aux->notifications, NULL);
    return NULL;
}

/**
 * @brief Start Director thread.
 *        The behaviour is undefined if p_d has not been previously initialized with #Director_init.
 * 
 * @param p_d Requirements: p_d != NULL and must refer to a Director object created with #Director_init. Target Director.
 * @return int: result pf pthread_create call
 */
int Director_startThread(Director * p_d) {
    return pthread_create(&p_d->thread, NULL, Director_main, p_d);
}

/**
 * @brief Join the director thread
 * 
 * @param p_d Requirements: p_d != NULL and must refer to a Director object created with #Director_init. Target Director.
 * @return int result of pthread_join
 */
int Director_joinThread(Director * p_d) {
    return pthread_join(p_d->thread, NULL);
}

/**
 * @brief Dealloc a Director object.
 *
 * @warning This function should be called by only one thread when no other thread is working on p_d object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_d Requirements: p_d != NULL and must refer to a Director object created with #Director_init. Target Director.
 * @return int: result code:
 *  1: p_d != NULL and the deallocation proceed witout errors. 
 *  -1: p_d == NULL
 */
int Director_delete(Director * p_d) {
	if(p_d == NULL) return -1; 
    pthread_cond_destroy(&p_d->cv_Director_AuthNews);
    pthread_cond_destroy(&p_d->cv_Director_DesksNews);
    pthread_mutex_destroy(&p_d->lock);
    SQueue_deleteQueue(p_d->notifications, free);
	free(p_d);
	return 1;
}

/**
 * @brief Thread dedicated to auth queue 
 * 
 * @return void* 
 */
void * Director_handleAuth(void * p_arg) {
    Market * m = (Market *) p_arg;
    Director * d = m->director;
    SQueue * auth = m->usersAuthQueue;
    void * data = NULL;
    User * user = NULL;
    while (1) {
       	//Wait a closure signal or new user in auth queue to proceed
		Lock(&d->lock);
		while (sig_hup != 1 && sig_quit != 1 && SQueue_isEmpty(auth)==1) 
			pthread_cond_wait(&d->cv_Director_AuthNews, &d->lock);
		Unlock(&d->lock);
		if(sig_hup == 1 || sig_quit == 1) {        
            //Empties the user auth queue and wait until no other users are in the market
            while (SQueue_isEmpty(m->usersShopping) != 1) {
                if(SQueue_pop(auth, &data) == 1) {
                    user = (User *) data;
                    //Move user to exit
                    Market_moveToExit(m, user);
                }
            }
            break;
        }
        //Market is not closing
        if(SQueue_pop(auth, &data) == 1){
            user = (User *) data;
            printf("[Director]: user %d is authorized for exit.\n", user->id);
            //Move user to exit
            Market_moveToExit(m, user);
        }
    }

    return (void *) NULL;
}

/**
 * @brief Entry point for a Diretor thread.
 * 
 * Function to use on Director thread creation as entry point.  
 * @param p_arg argument passed to the Director thread. Diector type expected.
 * @return void* 
 */
void * Director_main(void * p_arg){
	Director * d = (Director *) p_arg;
    Market * m = d->market;
    void * data = NULL;
    CashDeskNotify * msg = NULL;
    CashDeskNotify ** lastReceivedMsg = NULL;
    int desksMsg = 0;
    int tryOpen = 0, tryClose = 0;
    int numDeskNoWork = 0; //counter for the number of desk with low amount of work
	pthread_t thAuthHandler;
	printf("[Director]: start of thread.\n");

    if((lastReceivedMsg = calloc(d->market->K, sizeof(CashDesk **))) == NULL)
        ERR_QUIT("Malloc error");

    //Create auxiliary thread for managing auth queue
    if(pthread_create(&thAuthHandler, NULL, Director_handleAuth, d->market) !=0)
        ERR_QUIT("[Director]: an error occurred during creation of authorizations handler thread."); 
    
    //Handle cashdesks notifications
    while (1) {
        //Wait a closure signal or new desk notification to proceed
		Lock(&d->lock);
		while (sig_hup != 1 && sig_quit != 1 && SQueue_isEmpty(d->notifications)==1) 
			pthread_cond_wait(&d->cv_Director_DesksNews, &d->lock);
		Unlock(&d->lock);
		
        if(sig_hup == 1 || sig_quit == 1) break;
        
        if(SQueue_pop(d->notifications, &data) == 1) {
            //New notification received
            msg = (CashDeskNotify *) data;
            printf("[Director]: received notification from desk %d\n", msg->id);

            if(lastReceivedMsg[msg->id] == NULL) desksMsg++;
            else free(lastReceivedMsg[msg->id]);

            lastReceivedMsg[msg->id] = msg;
            if(desksMsg == m->K) {//All desk have communicated their status. Now it's time to take a decision.
                desksMsg = 0;
                tryOpen = 0;
                tryClose = 0;
                numDeskNoWork = 0;
                //Check if it's time to close/open a desk
                for(int i=0;i<m->K;i++) {
                    if(lastReceivedMsg[i]->state == DESK_OPEN && lastReceivedMsg[i]->users<=1) numDeskNoWork++;
                    if(lastReceivedMsg[i]->state == DESK_OPEN && lastReceivedMsg[i]->users>=m->S2) tryOpen=1;
                }
                if(numDeskNoWork >= m->S1) tryClose = 1;
                
                if(tryOpen){
                    //Try to open a desk
                    printf("[Director]: Try to open a desk\n");
                    PayArea_tryOpenDesk(m->payArea);
                }
                if(tryClose){
                    //Try to close a desk
                    printf("[Director]: Try to close a desk\n");
                    PayArea_tryCloseDesk(m->payArea);
                }
                //Reset
                for(int i=0;i<m->K;i++) {free(lastReceivedMsg[i]); lastReceivedMsg[i] = NULL;}
            }
        }

    }
    
    for(int i=0;i<m->K;i++) 
        if(lastReceivedMsg[i] != NULL) free(lastReceivedMsg[i]);
    free(lastReceivedMsg);
    
    if(pthread_join(thAuthHandler, NULL) !=0)
        ERR_QUIT("[Director]: an error occurred during join of authorizations handler thread."); 

	printf("[Director]: end of thread.\n");
    return (void *)NULL;
}
