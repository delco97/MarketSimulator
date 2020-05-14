#ifndef	_TCASHDESK_H
#define	_TCASHDESK_H

#include <pthread.h>
#include <SQueue.h>

typedef struct _CashDesk {
    pthread_mutex_t lock;  /**< lock variable */
    
    SQueue usersPay; /**< Users waiting for payment. */
} CashDesk;

#endif	/* _TCASHDESK_H */