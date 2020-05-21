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
    Market * market;  /**< Reference to the market where the director is. */
};

Director * Director_init(Market * m);
int Director_startThread(Director * p_d);
int Director_joinThread(Director * p_d);
int Director_delete(Director * p_d);
void * Director_main(void * p_arg);

//Getters
Market * Director_getMarket(Director * p_d);

#endif	/* _TDIRECTOR_H */