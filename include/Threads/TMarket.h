#ifndef	_TMARKET_H
#define	_TMARKET_H

#include <pthread.h>
#include <TDirector.h>
#include <SQueue.h>

#define MARKET_NAME_MAX 100

typedef struct _Market {
    pthread_t thread;   /**< Market  thread */
    pthread_mutex_t lock;  /**< lock variable */
    char name[MARKET_NAME_MAX]; /**< Name of the market */
    int C; /**< Maximum number of users allowed inside */
    int E;  /**< Number of users who need to exit before let other E users in.  */  
    Director director;  /**< Director of the market */
    SQueue usersShopping;  /**< Users in shopping area */

} Market;

Market * Market_init();
int Market_startThread(Market * p_u);
int Market_joinThread(Market * p_u);
int Market_delete(Market * p_u);


#endif	/* _TMARKET_H */
