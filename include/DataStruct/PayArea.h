/**
 * @file PayArea.h
 * @brief Header file of PayArea.c
 */

#ifndef PAYAREA_H
#define PAYAREA_H

#include <TMarket.h>
#include <TCashDesk.h>


typedef struct Market Market;
typedef struct CashDesk CashDesk;
typedef struct PayArea PayArea;

struct PayArea {
    pthread_mutex_t lock;  /**< lock variable */
    Market * p_m;    /**< market where the payment area is located */
    int nTot;    /**< number of all desks */
    int nOpen;  /**< number of open desks*/
    int nClose; /**< number of closed desk*/
    CashDesk ** desks; /**< Array of cashdesk */ 
};

PayArea * PayArea_init(Market * p_m, int p_tot, int p_open);
void PayArea_delete(PayArea * p_a);
int PayArea_isEmpty(PayArea *p_a);
void PayArea_Signal(PayArea *p_a);
void PayArea_startDeskThreads(PayArea *p_a);
void PayArea_joinDeskThreads(PayArea *p_a);

void PayArea_Lock(PayArea * p_a);
void PayArea_Unlock(PayArea * p_a);

#endif	/* PAYAREA_H */