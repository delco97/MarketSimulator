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

enum UserState {
    USR_READY,
    USR_NOT_READY,
    USR_QUIT
};

/**
 * @brief Data structure used to store information about a user.
 * 
 */
struct User {
    pthread_t thread;   /**< User thread */
    pthread_mutex_t lock;  /**< lock variable */
    pthread_cond_t cv_UserNews; /**< used to notify updates to User thread */
    int id; /**< Numberic identification number. */
    UserState state; /**< current user state */
    int products;  /**< Number of products in cart. */  
    int queueChanges; /**< Number of queue changed. */
    struct timespec tMarketEntry;  /**< Entry time in the market */
    struct timespec tMarketExit;  /**< Exit time from the market */
    struct timespec tQueueStart;  /**< Time when users start to wait in a queue to pay o to be authorized for exit*/
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

void * User_main(void * arg);

#endif	/* _TUSER_H */