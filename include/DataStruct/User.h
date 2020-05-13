#ifndef	_USER_H
#define	_USER_H

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

User * User_init(int p_products, int p_shoppingTime);

int getId(User * p_u);
int getProducts(User * p_u);
int getQueueChanges(User * p_u);
int getShoppingTime(User * p_u);
int getCashDeskTime(User * p_u);
int getAuthTime(User * p_u);

void changeQueue(User * p_u);

#endif	/* _USER_H */