/**
 * @file PayArea.c
 * @brief   A PayArea is a mutable thread safe object which models a payment area made of multiple desk for payments.
 */
#include <stdlib.h>
#include <utilities.h>
#include <PayArea.h>
#include <TCashDesk.h>

/**
 * @brief Create a new PayArea object.
 * 
 * @param p_m Market in which the PayArea operate
 * @param p_tot total number of desk in PayArea
 * @param p_open Requirements: 0 < p_open <= p_tot. Number of open desk (p_tot-p_open is the number of closed desks) 
 * @return PayArea *: new allocate object
 */
PayArea * PayArea_init(Market * p_m, int p_tot, int p_open) {
    PayArea * aux = NULL;
    if(p_m == NULL) err_quit("p_m == NULL");
    if(p_tot <= 0 || p_open <= 0)  err_quit("Invalid p_open or p_tot value. p_open: %d; p_tot: %d", p_open, p_tot);
    if(p_open <= 0 || p_open > p_tot) err_quit("Invalid p_open value. p_open: %d; p_tot: %d", p_open, p_tot);
    
   	if( (aux = malloc(sizeof(PayArea))) == NULL)
        err_quit("An error occurred during memory allocation. (payarea malloc)");
    
    //Init array of desks
	if( (aux->desks = malloc(p_tot * sizeof(CashDesk *))) == NULL)
		err_quit("An error occurred during memory allocation. (cashdesks array malloc)");
    aux->nTot = p_tot;
    aux->nOpen = p_open;
    aux->nClose = p_tot - p_open;        
	//Init all desks
	for(int i = 0;i < aux->nTot; i++) {
		if( (aux->desks[i] = CashDesk_init(p_m, i, getRandom(20, 80, &p_m->seed), (i<p_tot) ? DESK_OPEN:DESK_CLOSE)) == NULL )
			err_quit("An error occurred during cashdesk creation. Impossible to setup the market.");
		
	}
    //Init lock system
	if (pthread_mutex_init(&(aux->lock), NULL) != 0)
		err_msg("An error occurred during locking system initialization. Impossible to setup CashDesk.");
	
    return aux;
}

/**
 * @brief Dealloc a PayArea object.
 * 
 * @warning This function should be called by only one thread when no other thread is working on p_c object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_delete(PayArea * p_a) {
    if(p_a == NULL) err_quit("An error occurred during PayArea deletion. p_a == NULL.");
    pthread_mutex_destroy(&p_a->lock);
	for(int i = 0;i < p_a->nTot; i++) CashDesk_delete(p_a->desks[i]);
	free(p_a->desks);
    free(p_a);
}

/**
 * @brief Check if the payarea is empty (no users)
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 * @return int: result code:
 * 1: is empty
 * otherwise: is NOT empty
 */
int PayArea_isEmpty(PayArea *p_a) {
    int res_fun = 1;
    if(p_a == NULL) err_quit("p_a == NULL");
	PayArea_Lock(p_a);
    for(int i = 0;i < p_a->nTot && res_fun == 1;i++) 
		res_fun = SQueue_isEmpty(p_a->desks[i]->usersPay)!=1 ? 0:res_fun;
	PayArea_Unlock(p_a);
    return res_fun;
}

/**
 * @brief Start all desk threads.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_startDeskThreads(PayArea *p_a) {
	if(p_a == NULL) err_quit("p_a == NULL");
    
	for(int i = 0; i < p_a->nTot; i++){
		if(CashDesk_startThread(p_a->desks[i]) != 0)
			err_quit("[Market]: An error occurred during desk thread start. (CashDesk startThread failed)");
	}
}

/**
 * @brief Join all desk threads.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_joinDeskThreads(PayArea * p_a) {
	if(p_a == NULL) err_quit("p_a == NULL");
        
    for(int i = 0;i < p_a->nTot;i++) 
        if(CashDesk_joinThread(p_a->desks[i])!=0) err_quit("An error occurred during cash desk thread join.");    
}


/**
 * @brief Send signal to all desks.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_Signal(PayArea *p_a) {
    if(p_a == NULL) err_quit("p_a == NULL");
    for(int i=0;i<p_a->nTot;i++) Signal(&p_a->desks[i]->cv_DeskNews);
}


void PayArea_Lock(PayArea * p_a) {Lock(&p_a->lock);}
void PayArea_Unlock(PayArea * p_a) {Unlock(&p_a->lock);}
