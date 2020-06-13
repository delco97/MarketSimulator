/**
 * @file TDirector.h
 * @brief Header file for TDirector.c
 */
#ifndef	_TDIRECTOR_H
#define	_TDIRECTOR_H

#include <pthread.h>
#include <signal.h>
#include <SQueue.h>
#include <TCashDesk.h>
#include <TMarket.h>
#define DIRECTOR_NAME_MAX 100

typedef struct Market Market;
typedef struct Director Director;
extern volatile sig_atomic_t sig_hup;
extern volatile sig_atomic_t sig_quit;

struct Director {
    pthread_t thread;   /**< Director thread */
    pthread_mutex_t lock; /**< lock variable */
    Market * market;  /**< Reference to the market where the director is. */
    pthread_cond_t cv_Director_AuthNews; /**< used to notify updates to Director thread that handles auth queue */
    pthread_cond_t cv_Director_DesksNews; /**< used to notify updates to Director thread that handles cash desk notifications*/
    SQueue * notifications; /**< notification received from cashdesks */
};

Director * Director_init(Market * m);
int Director_startThread(Director * p_d);
int Director_joinThread(Director * p_d);
int Director_delete(Director * p_d);
void * Director_main(void * p_arg);
void Director_Lock(Director * p_d);
void Director_Unlock(Director * p_d);

//Getters
Market * Director_getMarket(Director * p_d);

#endif	/* _TDIRECTOR_H */