#ifndef	_TCASHDESK_H
#define	_TCASHDESK_H

#include <pthread.h>
#include <SQueue.h>
#include <TMarket.h>

typedef struct Market Market;
typedef struct CashDesk CashDesk;
typedef struct User User;
typedef enum CashDeskState CashDeskState;

enum CashDeskState {
    DESK_OPEN,
    DESK_CLOSE
};

struct CashDesk {
    pthread_t thread;   /**< CaskDesk thread */
    pthread_mutex_t lock;  /**< lock variable */
    pthread_cond_t cv_canServe; /**< condition variable used to know if anybody can be served*/
    int id; /** desk id */
    CashDeskState state;    /**< current cashdesk state */
    SQueue * usersPay; /**< Users waiting for payment. */
    Market * market;  /**< Reference to the market where the director is. */
};

int CashDesk_init(Market * p_m, CashDesk * p_c, int p_id, CashDeskState p_state);
int CashDesk_delete(CashDesk * p_c);
int CashDesk_startThread(CashDesk * p_c);
int CashDesk_joinThread(CashDesk * p_c);
void * CashDesk_main(void * p_arg);
void CashDesk_Lock(CashDesk * p_m);
void CashDesk_Unlock(CashDesk * p_m);
//Getters
CashDeskState CashDesk_getId(CashDesk * p_c);
CashDeskState CashDesk_getSate(CashDesk * p_c);
SQueue * CashDesk_getUsersPay(CashDesk * p_c);
Market * CashDesk_getMarket(CashDesk * p_c);
//Setters
void CashDesk_addUser(CashDesk * p_c, User * p_u);

#endif	/* _TCASHDESK_H */