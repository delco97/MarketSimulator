#ifndef	_TUSER_H
#define	_TUSER_H

#include <SQueue.h>
#include <Market.h>
#include <pthread.h>

typedef struct _User {
    pthread_mutex_t lock;  /**< lock variable */
    int id; /**< Numberic identification number. */
    int products;  /**< Number of products in cart. */  
    int queueChanges; /**< Number of queue changed. */
    int shoppingTime; /**< Time to spend in shopping area. */
    int cashDeskTime; /**< Time spent in a cash desk queue. */
    int authTime;   /**< Time spent before getting the exit authorization. */
} User;

/**
 * @brief Arguments passed to thread TUser.c
 */
typedef struct _TUserArg {
    int id; /**< Custom thread id. */
    User * u; /**< User associated to the thread. */
    Market * m;  /**< Reference to the market where the user is. */
} TUserArg;


User * User_init(int p_products, int p_shoppingTime);

int getId(User * p_u);
int getProducts(User * p_u);
int getQueueChanges(User * p_u);
int getShoppingTime(User * p_u);
int getCashDeskTime(User * p_u);
int getAuthTime(User * p_u);
void changeQueue(User * p_u);
void * th_Tokenizer_main(void * arg);

#endif	/* _TUSER_H */