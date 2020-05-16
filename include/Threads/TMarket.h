#ifndef	_TMARKET_H
#define	_TMARKET_H

#include <pthread.h>
#include <TDirector.h>
#include <SQueue.h>

#define MARKET_NAME_MAX 100

typedef struct _Market {
    pthread_t thread;   /**< Market  thread */
    pthread_mutex_t lock;  /**< lock variable */
    //Global variables configured according to the config file on startup.
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
    Director director;  /**< Director of the market */
    SQueue usersShopping;  /**< Users in shopping area */

} Market;

Market * Market_init(const char * p_conf, const char * p_log);
int Market_startThread(Market * p_u);
int Market_joinThread(Market * p_u);
int Market_delete(Market * p_u);


#endif	/* _TMARKET_H */
