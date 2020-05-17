#ifndef	_TCASHDESK_H
#define	_TCASHDESK_H

#include <pthread.h>
#include <SQueue.h>
typedef struct _CashDesk CashDesk;
struct CashDesk {
    pthread_t thread;   /**< CaskDesk thread */
    pthread_mutex_t lock;  /**< lock variable */
    
    SQueue usersPay; /**< Users waiting for payment. */
};

CashDesk CashDesk_init();

#endif	/* _TCASHDESK_H */