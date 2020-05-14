#ifndef	_TMARKET_H
#define	_TMARKET_H

#include <pthread.h>
#include <TDirector.h>
#include <SQueue.h>

#define MARKET_NAME_MAX 100



typedef struct _Market {
    pthread_mutex_t lock;  /**< lock variable */
    char name[MARKET_NAME_MAX]; /**< Name of the market */
    int C; /**< Maximum number of users allowed inside */
    int E;  /**< Number of users who need to exit before let other E users in.  */  
    Director director;  /**< Director of the market */
    SQueue usersShopping;  /**< Users in shopping area */

} Market;

Market * Market_init(int p_K, int p_C, int p_E, int p_T, int p_P, int p_S1, int p_S2, int p_NP, char * p_logPath);


#endif	/* _TMARKET_H */
