/**
 * @file utilities.c
 * @brief	Contains all common utilies function and macros used in the project.
 */

#include <utilities.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <time.h>

_Thread_local unsigned int g_seed; /**< Seed variable defined for each thread (_Thread_local) used 
                                        by rand_r calls performed within #utilities.getRandom.
                                        IMPORTANT: must be initialized.*/

/**
 * @brief Wait a specified amount of milliseconds.
 * 
 * This code has been taken from https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds.
 * 
 * @param p_waitMs milliseconds to wait.
 * @return int: error code:
 * -1: an error occurred (errno set)
 * 	0: successfully executed
 */
int waitMs(long p_msec){
    struct timespec ts;
    int res;
	//Check input
    if (p_msec < 0){
        errno = EINVAL;
        return -1;
    }
	//Conert p_msec in second and nanoseconds
    ts.tv_sec = p_msec / 1000;
    ts.tv_nsec = (p_msec % 1000) * 1000000;

	//if nanosleep is interrupted by a signal handler
	//continue to wait from where it stopped.
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

	//Get here only if nanosleep has finished its task or if an error occurred.
    return res;
}

/**
 * @brief Return the elapsed (p_end - p_start) time in ms.
 * 
 * @param p_start start time
 * @param p_end end time
 * @return long 
 */
long elapsedTime(struct timespec p_start, struct timespec p_end) {
	return (p_end.tv_sec - p_start.tv_sec) * 1000 + (p_end.tv_nsec - p_start.tv_nsec) / 1000000;
}
/**
 * @brief Get the current time as struct timespec
 * @return struct timespec set with current time
 */
struct timespec getCurrentTime(){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return now;
}

//General stuff
/**
 * @brief 	Get a random integer value in the following range [p_lower; p_upper]
 * 			Remember to set a seed using srand before.
 * 
 * @param p_lower smallest number that can be produced.
 * @param p_upper biggest number that can be produced.
 * @return int: generated number
 */
int getRandom(int p_lower, int p_upper) {
    return (rand_r(&g_seed) % (p_upper - p_lower + 1)) + p_lower; 
}

//Locking utilities
void Lock(pthread_mutex_t * p_lock) {if((pthread_mutex_lock(p_lock)) != 0) ERR_QUIT("An error occurred during locking.");}
void Unlock(pthread_mutex_t * p_lock) {if(pthread_mutex_unlock(p_lock) != 0) ERR_QUIT("An error occurred during unlocking.");}
void Wait(pthread_cond_t * p_cond, pthread_mutex_t * p_lock) {if(pthread_cond_wait(p_cond, p_lock) != 0) ERR_QUIT("An error occurred during cond wait.");}
void Signal(pthread_cond_t * p_cond) {if(pthread_cond_signal(p_cond) != 0) ERR_QUIT("An error occurred during a condition singal.");}
void Broadcast(pthread_cond_t * p_cond) {if(pthread_cond_broadcast(p_cond) != 0) ERR_QUIT("An error occurred during a brodcast.");}

