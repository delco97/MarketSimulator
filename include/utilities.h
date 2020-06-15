/**
 * @file utilities.h
 * @brief Header file for utilities.c
 */
#ifndef	_UTILITIES_H
#define	_UTILITIES_H

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define	MAXLINE	4096			/* max line length for messages*/

extern _Thread_local unsigned int g_seed;

//**Debug messages **
#ifdef _DEBUG
	#define DEBUG_PRINT(M, ...) \
        fprintf(stdout, "[INFO] (%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define DEBUG_PRINT(M, ...) do {} while (0)
#endif

//** Log/Error handling macros **
#define ERR_MSG(M, ...) \
	fprintf(stderr, "[ERROR] (%s:%d): " M, __FILE__, __LINE__, ##__VA_ARGS__);\

#define ERR_QUIT(M, ...) \
	do {\
		fprintf(stderr, "[ERROR] (%s:%d): " M, __FILE__, __LINE__, ##__VA_ARGS__);\
		exit(1);\
	} while(0)

#define ERR_SYS_MSG(M, ...) \
	do {\
		fprintf(stderr, "[ERROR] (%s:%d): " M, __FILE__, __LINE__, ##__VA_ARGS__);\
		perror("System call error");\
	} while(0)

#define ERR_SYS_QUIT(M, ...) \
	do {\
		fprintf(stderr, "[ERROR] (%s:%d): " M, __FILE__, __LINE__, ##__VA_ARGS__);\
		perror("System call error");\
		exit(1);\
	} while(0)

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
int getRandom(int p_lower, int p_upper);

#endif	/* _UTILITIES_H */
