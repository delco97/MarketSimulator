#ifndef	_TDIRECTOR_H
#define	_TDIRECTOR_H

#include <pthread.h>
#include <SQueue.h>
#include <TCashDesk.h>
#include <TMarket.h>
#define DIRECTOR_NAME_MAX 100

typedef struct Market Market;
typedef struct Director Director;

struct Director {
    pthread_t thread;   /**< Director thread */
    Market * market;  /**< Reference to the market where the director is. */
    SQueue usersAuthQueue; /**< Users waiting for authorization. */
    CashDesk * desks; /**< Array of cash desks managed by director */
};

Director * Director_init(Market * m);
int Director_startThread(Director * p_u);
int Director_joinThread(Director * p_u);
int Director_delete(Director * p_u);

#endif	/* _TDIRECTOR_H */