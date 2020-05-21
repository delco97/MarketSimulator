/**
 * @file TUser.h
 * @brief Header file for TUser.c
 */
#ifndef	_TUSER_H
#define	_TUSER_H

#include <SQueue.h>
#include <signal.h>
#include <TMarket.h>
#include <pthread.h>
#include <time.h>

typedef enum UserState UserState;
typedef struct Market Market;
typedef struct User User;
extern volatile sig_atomic_t sig_hup;
extern volatile sig_atomic_t sig_quit;

struct User {
    pthread_t thread;   /**< User thread */
    pthread_mutex_t lock;  /**< lock variable */
    int id; /**< Numberic identification number. */
    int products;  /**< Number of products in cart. */  
    int queueChanges; /**< Number of queue changed. */
    struct timespec tMarketEntry;  /**< Entry time in the market */
    struct timespec tMarketExit;  /**< Exit time from the market */
    struct timespec tQueueStart;  /**< Time when users start to wait in a queue to pay o to be authorized for exit*/
    struct timespec tStartPayment;  /**< Time when a cashier start to serve the user */
    int shoppingTime; /**< Time to spend in shopping area in ms. */
    Market * market;  /**< Reference to the market where the user is. */
};


User * User_init(int p_products, int p_shoppingTime, Market * p_m);
int User_startThread(User * p_u);
int User_joinThread(User * p_u);
int User_delete(User * p_u);
void User_reset(User * p_u, int p_products, int p_shoppingTime, Market * p_m);
void User_log(User * p_u);
int User_compare(void * p_u1, void * p_u2);
//Getters
int User_getId(User * p_u);
int User_getProducts(User * p_u);
int User_getQueueChanges(User * p_u);
struct timespec User_getMarketEntryTime(User * p_u);
struct timespec User_getMarketExitTime(User * p_u);
struct timespec User_getQueueStartTime(User * p_u);
struct timespec User_getStartPaymentTime(User * p_u);
int User_getShoppingTime(User * p_u);
Market * User_getMarket(User * p_u);
//Setters
void User_setMarketEntryTime(User * p_u, struct timespec p_x);
void User_setMarketExitTime(User * p_u, struct timespec p_x);
void User_setQueueStartTime(User * p_u, struct timespec p_x);
void User_setStartPaymentTime(User * p_u, struct timespec p_x);
void User_setProducts(User * p_u, int p_prd);
void User_changeQueue(User * p_u);

void * User_main(void * arg);

#endif	/* _TUSER_H */