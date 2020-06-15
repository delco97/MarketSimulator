/**
 * @file PayArea.c
 * @brief   A PayArea is a mutable thread safe object which models a payment area made of multiple desk for payments.
 */
#include <stdlib.h>
#include <utilities.h>
#include <PayArea.h>
#include <TCashDesk.h>

//Private functions
static CashDesk * pGetRandomDesk(PayArea *p_a, CashDeskState p_state) {
	//Choose a random open cashk desk
	SQueue * selectedDesks = NULL;
	void * aux = NULL;
	CashDesk * deskChoosen = NULL;
	if( (selectedDesks = SQueue_init(-1)) == NULL) 
		ERR_QUIT("Malloc error.");
	for(int i=0;i < p_a->nTot; i++) {//Get id of all desks currently open
		if(p_a->desks[i]->state == p_state){
			if(SQueue_push(selectedDesks, p_a->desks[i]) != 1)
				ERR_QUIT("An error occurred during desk search. (1)");
		}
	}
	int r = getRandom(0,SQueue_dim(selectedDesks)-1, &p_a->market->seed);
	if(SQueue_removePos(selectedDesks, r, &aux) != 1)
		ERR_QUIT("An error occurred during desk search. (2)");
	SQueue_deleteQueue(selectedDesks, NULL);
	deskChoosen = (CashDesk *) aux;
    return deskChoosen;
}

// static CashDesk * pGetLessBusyDesk(PayArea *p_a) {
// 	//Choose desk with min number o users in queue
// 	CashDesk * selectedDesk = NULL;

//     //Select first open desk
//     for(int i=0;i < p_a->nTot; i++) {
//         if(p_a->desks[i]->state == DESK_OPEN){
//             selectedDesk = p_a->desks[i];
//             break;
//         }
//     }
//     //Find desk with min number of users in queue
// 	for(int i=0;i < p_a->nTot; i++) {//Get id of all desks currently open
// 		if(p_a->desks[i]->state == DESK_OPEN && SQueue_dim(p_a->desks[i]->usersPay) < SQueue_dim(selectedDesk->usersPay)) 
//             selectedDesk = p_a->desks[i];
// 	}

//     return selectedDesk;
// }

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
    if(p_m == NULL) ERR_QUIT("p_m == NULL");
    if(p_tot <= 0 || p_open <= 0)  ERR_QUIT("Invalid p_open or p_tot value. p_open: %d; p_tot: %d", p_open, p_tot);
    if(p_open <= 0 || p_open > p_tot) ERR_QUIT("Invalid p_open value. p_open: %d; p_tot: %d", p_open, p_tot);
    
   	if( (aux = malloc(sizeof(PayArea))) == NULL)
        ERR_QUIT("An error occurred during memory allocation. (payarea malloc)");
    
    //Init array of desks
	if( (aux->desks = malloc(p_tot * sizeof(CashDesk *))) == NULL)
		ERR_QUIT("An error occurred during memory allocation. (cashdesks array malloc)");
    aux->nTot = p_tot;
    aux->nOpen = p_open;
    aux->nClose = p_tot - p_open;      
    aux->market = p_m;  
	//Init all desks
	for(int i = 0;i < aux->nTot; i++) {
		if( (aux->desks[i] = CashDesk_init(p_m, i, p_m->TD, getRandom(20, 80, &p_m->seed), (i<p_open) ? DESK_OPEN:DESK_CLOSE)) == NULL )
			ERR_QUIT("An error occurred during cashdesk creation. Impossible to setup the market.");
		
	}
    //Init lock system
	if (pthread_mutex_init(&(aux->lock), NULL) != 0)
		ERR_MSG("An error occurred during locking system initialization. Impossible to setup CashDesk.");
	
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
    if(p_a == NULL) ERR_QUIT("An error occurred during PayArea deletion. p_a == NULL.");
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
    if(p_a == NULL) ERR_QUIT("p_a == NULL");
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
	if(p_a == NULL) ERR_QUIT("p_a == NULL");
    
	for(int i = 0; i < p_a->nTot; i++){
		if(CashDesk_startThread(p_a->desks[i]) != 0)
			ERR_QUIT("[Market]: An error occurred during desk thread start. (CashDesk startThread failed)");
	}
}

/**
 * @brief Join all desk threads.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_joinDeskThreads(PayArea * p_a) {
	if(p_a == NULL) ERR_QUIT("p_a == NULL");
        
    for(int i = 0;i < p_a->nTot;i++) 
        if(CashDesk_joinThread(p_a->desks[i])!=0) ERR_QUIT("An error occurred during cash desk thread join.");    
}


/**
 * @brief Send signal to all desks.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 */
void PayArea_Signal(PayArea *p_a) {
    if(p_a == NULL) ERR_QUIT("p_a == NULL");
    for(int i=0;i<p_a->nTot;i++) Signal(&p_a->desks[i]->cv_DeskNews);
}

/**
 * @brief Try to open a desk. This works only if there are less then nOpen<nTot.
 * 
 * @param p_a 
 */
void PayArea_tryOpenDesk(PayArea *p_a) {
    CashDesk * selected = NULL;    
    PayArea_Lock(p_a);
    if(p_a->nOpen != p_a->nTot) {
        selected = pGetRandomDesk(p_a, DESK_CLOSE);
        selected->state = DESK_OPEN;
        p_a->nOpen++;
        p_a->nClose--;
        Signal(&selected->cv_DeskNews);
    }
    PayArea_Unlock(p_a);
}

/**
 * @brief Try to close a desk. This works only if there are at least 2 desks open.
 * 
 * @param p_a 
 */
void PayArea_tryCloseDesk(PayArea *p_a) {
    CashDesk * closedDesk = NULL;
    CashDesk * moveToDesk = NULL;
    void * data = NULL;
    User * aux = NULL;
    PayArea_Lock(p_a);
    if(p_a->nOpen >= 2) {
        //closedDesk = pGetLessBusyDesk(p_a); //Removed because director tend to close always the same desk.
        closedDesk = pGetRandomDesk(p_a, DESK_OPEN);
        closedDesk->state = DESK_CLOSE;
        p_a->nOpen--;
        p_a->nClose++;
        //Move all users in queue to other randomly choosen open desks (is always possible to find one)
        while (SQueue_pop(closedDesk->usersPay, &data) == 1) {
            moveToDesk = pGetRandomDesk(p_a, DESK_OPEN);
            aux = (User *) data;
            aux->queueChanges++;	
            CashDesk_addUser(moveToDesk, aux);
        }
        
        Signal(&closedDesk->cv_DeskNews);
    }
    PayArea_Unlock(p_a);
}

/**
 * @brief Add a new user to one randomly choosen open desk.
 * 
 * @param p_a Requirements: p_a != NULL and must refer to a PayArea object created with #PayArea_init.
 * @param p_u Requirements: p_u != NULL and must refer to a User object created with #User_init. Target User.
 */
void PayArea_addUser(PayArea * p_a, User * p_u) {
	CashDesk * deskChoosen = NULL;	
    PayArea_Lock(p_a);
	deskChoosen = pGetRandomDesk(p_a, DESK_OPEN);
    p_u->tQueueStart = getCurrentTime();
    p_u->queueChanges++;
	CashDesk_addUser(deskChoosen, p_u);
	Signal(&deskChoosen->cv_DeskNews);
	PayArea_Unlock(p_a);
}

void PayArea_Lock(PayArea * p_a) {Lock(&p_a->lock);}
void PayArea_Unlock(PayArea * p_a) {Unlock(&p_a->lock);}
