#ifndef	_TUSER_H
#define	_TUSER_H

#include <SQueue.h>
#include <User.h>

/**
 * @brief Arguments passed to thread TUser.c
 */
typedef struct _TUserArg {
    int id; /**< Custom thread id */
    User u; /**< User associated to the thread */
    SQueue * in; /**< input queue to process */
    SQueue * out; /**< Queue of output results */
} TUserArg;

void * th_Tokenizer_main(void * arg);

#endif	/* _TUSER_H */