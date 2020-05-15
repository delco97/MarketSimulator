#ifndef	_TDIRECTOR_H
#define	_TDIRECTOR_H

#include <pthread.h>
#include <SQueue.h>
#include <TCashDesk.h>
#define DIRECTOR_NAME_MAX 100

typedef struct _Director {
    pthread_t thread;   /**< Director thread */
    int S1; /**< (S1) threshold for cashdesk closure. */
    int S2; /**< (S2) threshold for cashdesk opening. */
    int K; /**< (K) maximum number of cashdesks open */
    SQueue usersAuthQueue; /**< Users waiting for authorization. */
    CashDesk * desks; /**< Array of cash desks managed by director */
} Director;

Director * Director_init(char * p_path_config);
int Director_startThread(Director * p_u);
int Director_joinThread(Director * p_u);
int Director_delete(Director * p_u);

#endif	/* _TDIRECTOR_H */