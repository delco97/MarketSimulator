/**
 * @file utilities.h
 * @brief Header file for utilities.c
 */
#ifndef	_UTILITIES_H
#define	_UTILITIES_H

#include <time.h>
#include <pthread.h>
#include <stdio.h>

#define	MAXLINE	4096			/* max line length for messages*/

//**Debug messages **
#ifdef _DEBUG
	#define DEBUG_PRINT(M, ...) \
        fprintf(stdout, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define DEBUG_PRINT(M, ...) do {} while (0)
#endif

//** Log/Error handling functions **
void	err_msg(const char *, ...);	
void	err_dump(const char *, ...) __attribute__((noreturn));
void	err_quit(const char *, ...) __attribute__((noreturn));
void	err_cont(int, const char *, ...);
void	err_exit(int, const char *, ...) __attribute__((noreturn));
void	err_ret(const char *, ...);
void	err_sys(const char *, ...) __attribute__((noreturn));

//** Time utilities
int waitMs(long p_msec);
long elapsedTime(struct timespec p_start, struct timespec p_end);
struct timespec getCurrentTime();

//** Lock/Unlock utilities
void Lock(pthread_mutex_t * p_lock);
void Unlock(pthread_mutex_t * p_lock);
void Wait(pthread_cond_t * p_cond, pthread_mutex_t * p_lock);
void Signal(pthread_cond_t * p_cond);
void Broadcast(pthread_cond_t * p_cond);

//** General utilities
int getRandom(int p_lower, int p_upper, unsigned int * p_seed);

#endif	/* _UTILITIES_H */
