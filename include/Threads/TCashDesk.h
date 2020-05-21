/**
 * @file TCashDesk.h
 * @brief Header file for TCashDesk.c
 */
#ifndef	_TCASHDESK_H
#define	_TCASHDESK_H

#include <pthread.h>
#include <signal.h>
#include <SQueue.h>
#include <TMarket.h>

typedef struct Market Market;
typedef struct CashDesk CashDesk;
typedef struct User User;
typedef enum CashDeskState CashDeskState;
extern volatile sig_atomic_t sig_hup;
extern volatile sig_atomic_t sig_quit;

enum CashDeskState {
    DESK_OPEN,
    DESK_CLOSE
};

struct CashDesk {
    pthread_t thread;   /**< CaskDesk thread */
    pthread_mutex_t lock;  /**< lock variable */
    int id; /** desk id */
    int serviceConst; /**< costant service time */
    int productsProcessed; /**< number of products processed */
    int usersProcessed; /**< number of users served */
    int numClosure; /**< number of closure */
    int totOpenTime; /**< tot open time in ms */
    float avgSeviceTime; /**< average service time for a user*/
    CashDeskState state;    /**< current cashdesk state */
    SQueue * usersPay; /**< Users waiting for payment. */
    Market * market;  /**< Reference to the market where the director is. */
};

int CashDesk_init(Market * p_m, CashDesk * p_c, int p_id, int p_serviceConst, CashDeskState p_state);
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

void CashDesk_log(CashDesk * p_c);

#endif	/* _TCASHDESK_H */