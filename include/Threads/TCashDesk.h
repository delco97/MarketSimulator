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
typedef struct CashDeskNotify CashDeskNotify;
typedef enum CashDeskState CashDeskState;
extern volatile sig_atomic_t sig_hup;
extern volatile sig_atomic_t sig_quit;

enum CashDeskState {
    DESK_OPEN,
    DESK_CLOSE
};

/**
 * @brief Data structure used to periodically notify
 * director thread about a cash desk status.
 */
struct CashDeskNotify {
    int id; //**< cash desk id who sent this notify*/
    int users; //**< users in queue */
    CashDeskState state; //** desk state */
};

/**
 * @brief Data structure used to store information about a cash desk.
 * 
 */
struct CashDesk {
    pthread_t thread;   /**< CaskDesk thread */
    pthread_mutex_t lock;  /**< lock variable */
    pthread_cond_t cv_DeskNews; /**< used to notify updates to CashDesk thread */
    int id; /** desk id */
    int serviceConst; /**< costant service time */
    int productsProcessed; /**< number of products processed */
    int usersProcessed; /**< number of users served */
    int numClosure; /**< number of closure */
    int totOpenTime; /**< tot open time in ms */
    float avgServiceTime; /**< average service time for a user*/
    int notifyInterval; /**< notify interval to inform director thread in ms*/
    CashDeskState state;    /**< current cashdesk state */
    SQueue * usersPay; /**< Users waiting for payment. */
    Market * market;  /**< Reference to the market where the director is. */
};

CashDesk * CashDesk_init(Market * p_m, int p_id, int p_serviceConst, int p_notifyInterval, CashDeskState p_state);
int CashDesk_delete(CashDesk * p_c);
int CashDesk_startThread(CashDesk * p_c);
int CashDesk_joinThread(CashDesk * p_c);
void * CashDesk_notifyDirector(void * p_arg);
void * CashDesk_main(void * p_arg);
void CashDesk_Lock(CashDesk * p_m);
void CashDesk_Unlock(CashDesk * p_m);
void CashDesk_addUser(CashDesk * p_c, User * p_u);
void CashDesk_log(CashDesk * p_c);

#endif	/* _TCASHDESK_H */