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
    return aux;
err:
    if(aux != NULL) free(aux);
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
	free(p_d);
	return 1;
}

//Getters
Market * Director_getMarket(Director * p_d) {return p_d->market;}


/**
 * @brief Thread dedicated to auth queue 
 * 
 * @return void* 
 */
void * Director_handleAuth(void * p_arg) {
    Market * m = (Market *) p_arg;
    SQueue * auth = Market_getUsersAuth(m);
    void * data = NULL;
    User * user = NULL;
    while (1) {
        if(SQueue_popWait(auth, &data) != 1)
            err_quit("[Director]: an error occurred during authorization queue handling.");
        user = (User *) data;
        //Move user to exit
        Market_moveToExit(m, user);
    }
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
	pthread_t thAuthHandler;
	printf("[Director]: start of thread.\n");

    //Create auxiliary thread for managing auth queue
    if(pthread_create(&thAuthHandler, NULL, Director_handleAuth, d->market) !=0)
        err_quit("[Director]: an error occurred during creation of authorizations handler thread."); 
    //Handle cashdesks


    if(pthread_join(thAuthHandler, NULL) !=0)
        err_quit("[Director]: an error occurred during join of authorizations handler thread."); 

	printf("[Director]: end of thread.\n");
    return (void *)NULL;
}
