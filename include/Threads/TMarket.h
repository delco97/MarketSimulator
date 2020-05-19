#ifndef	_TMARKET_H
#define	_TMARKET_H

#include <pthread.h>
#include <TDirector.h>
#include <SQueue.h>
#include <TUser.h>
#include <Config.h>
#include <TCashDesk.h>

#define MARKET_NAME_MAX 100

typedef struct Market Market;
typedef struct Director Director;
typedef struct User User;
typedef struct CashDesk CashDesk;

struct Market {
    pthread_t thread;   /**< Market  thread */
    pthread_t thSignalHandler; /**< Thread used for signal handling */
    pthread_mutex_t lock;  /**< lock variable */
    unsigned int seed;  /**< seed used by the market thread for generating random number */
    long K; 	/**< Maximum number of open cashdesk. {K>0} */
    long KS; 	/**< Number of open cashdesks at opening. {0<KS<=K} */
    long C; 	/**< Maximum number of client allowed inside. {C >1} */
    long E; 	/**< Number of users who need to exit before let other E users in. {0<E<C} */
    long T; 	/**< Number of maximum ms spent by a single user in shopping area. {T>10} */
    long P; 	/**< Maximum number of products that a single user can buy. {P>0} */
    long S;	/**< Change queue evaluation interval (ms). {S>0}*/
    long S1; 	/**< This threshold set the limit that tells to director if it's time to close a cashdesk.
                    In particular, S1 is maximum number of cashdesk with at most one user in queue.
                    When S1 is exceeded, it's time to close a cashdesk. {S1>0}*/
    long S2; 	/**< This threshold set the limit that tells to director if it's time to open a cashdesk.
                    In particular, S2 is the maximum number of users in a single queue. So if there is at least
                    one queue with a number of user in queue equals or greater then S2. {S2>0} */
    long NP; 	/**< Time required to process a single product. {NP>0} */
    FILE * f_log; /**< FILE used for log simulation results*/
    Director * director;  /**< Director of the market */
    SQueue * usersShopping;  /**< Users in shopping area */
    SQueue * usersExit;  /**< Users who have left the market */
    SQueue * usersAuthQueue; /**< Users waiting for director authorization. */
    CashDesk * desks; /**< Array of cashdesk in the market */    
};

Market * Market_init(const char * p_conf, const char * p_log);
void * Market_main(void * arg);
int Market_startThread(Market * p_m);
int Market_joinThread(Market * p_m);
int Market_delete(Market * p_m);
void Market_Lock(Market * p_m);
void Market_Unlock(Market * p_m);


unsigned int * Market_getSeed(Market * p_m);
long Market_getK(Market * p_m);
long Market_getKS(Market * p_m);
long Market_getC(Market * p_m);
long Market_getE(Market * p_m);
long Market_getT(Market * p_m);
long Market_getP(Market * p_m);
long Market_getS(Market * p_m);
long Market_getS1(Market * p_m);
long Market_getS2(Market * p_m);
long Market_getNP(Market * p_m);
Director * Market_getDirector(Market * p_m);
SQueue * Market_getUsersShopping(Market * p_m);
SQueue * Market_getUsersExit(Market * p_m);
SQueue * Market_getUsersAuth(Market * p_m);
CashDesk * Market_getDesks(Market * p_m);

void Market_FromShoppingToPay(Market * p_m, User * p_u);
void Market_FromShoppingToAuth(Market * p_m, User * p_u);
void Market_moveToExit(Market * p_m, User * p_u);
void Market_log(Market * p_m, char * p_data);
#endif	/* _TMARKET_H */
