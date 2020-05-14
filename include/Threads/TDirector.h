#ifndef	_TDIRECTOR_H
#define	_TDIRECTOR_H

#include <pthread.h>
#include <SQueue.h>
#include <TCashDesk.h>
#define DIRECTOR_NAME_MAX 100

typedef struct _Director {
    char name[DIRECTOR_NAME_MAX]; /**< Name. */
    int S1; /**< (S1) threshold for cashdesk closure. */
    int S2; /**< (S2) threshold for cashdesk opening. */
    int K; /**< (K) maximum number of cashdesks open */
    SQueue usersAuthQueue; /**< Users waiting for authorization. */
    CashDesk * desks; /**< Array of cash desks managed by director */
} Director;

Director * Director_init(int p_threshHold_closure);
int Director_openCashDesk();

#endif	/* _TDIRECTOR_H */