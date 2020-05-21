/**
 * @file utilities.c
 * @brief	Implement functions defined in util.h.
 *			Contains all common utilies functions used in the project.
 * 
 *          This subset of utilities functions is derived from utilities used by the book:
 *          "Advanced Programming in the UNIX Environment 3rd Edition"
 *          They have been taken from here: http://www.apuebook.com/code3e.html.
 *          In particular, these functions comes from apue.h error.c and errorlog.c.
 *          Most of the code is the same, except for:
 * 				- Additional comments for doxygen and better explanation
 */

#include <utilities.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>		/**> For definition of errno and error functions*/
#include <stdarg.h>		/**> ISO C variable aruments */
#include <time.h>
#include <pthread.h>
#include <time.h>

/**
 * @brief   Print a message and return to caller.
 *          Caller specifies "errnoflag".
 * @param errnoflag tell if an error that set errno occurred.If errno != 0 means that errno will be used to get the associated errno message;
 *                  otherwise it won't be used.
 * @param error errno value to process. This parameter is considered only if errnoflag != 0.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ap list.
 * @param ap contains a list of values associated to placeholders (if any) in fmt.
 */
static void err_doit(int errnoflag, int error, const char *fmt, va_list ap) {
	char	buf[MAXLINE];
    //Loads the data from the locations, defined by ap, converts them to character string equivalents
    //and writes the results to a character string buffer (buff). At most buf_size - 1 characters are written
	vsnprintf(buf, MAXLINE-1, fmt, ap);
	if (errnoflag) //Check if errno must be evaluated in order to append errno message
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf)-1, ": %s", strerror(error));
	strcat(buf, "\n");  //Add new line at the end
	fflush(stdout);		/* in case stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(NULL);		/* flushes all stdio output streams */
}

/**
 * @brief   Nonfatal error related to a system call.
 *          Print a message and return.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_ret(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}

/**
 * @brief   Fatal error related to a system call.
 *          Print a message and terminate.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_sys(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

/**
 * @brief   Nonfatal error unrelated to a system call.
 *          Error code passed as explict parameter.
 *          Print a message and return.
 * @param error custom error id code.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_cont(int error, const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
}

/**
 * @brief   Fatal error unrelated to a system call.
 *          Error code passed as explict parameter.
 *          Print a message and terminate.
 * @param error custom error id code.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_exit(int error, const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}

/**
 * @brief   Fatal error related to a system call.
 *          Print a message, dump core, and terminate.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_dump(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

/**
 * @brief   Nonfatal error unrelated to a system call.
 *          Print a message and return.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_msg(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}

/**
 * @brief 
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 * @param fmt message to print. It can contains placeholders like printf (\%s, \%d, etc) and values associated to are in ...
 * @param ... contains a list of values associated to placeholders (if any) in fmt.
 */
void err_quit(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

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
 * 			Remember to set a seed using srand before. Each thread using this function
 * 			should have its own seed.
 * 
 * @param p_lower 
 * @param p_upper 
 * @return int: 
 */
int getRandom(int p_lower, int p_upper, unsigned int * p_seed) { 
    return (rand_r(p_seed) % (p_upper - p_lower + 1)) + p_lower; 
} 

//Locking utilities
void Lock(pthread_mutex_t * p_lock) {if(pthread_mutex_lock(p_lock) != 0) err_quit("An error occurred during locking.");}
void Unlock(pthread_mutex_t * p_lock) {if(pthread_mutex_unlock(p_lock) != 0) err_quit("An error occurred during unlocking.");}
void Wait(pthread_cond_t * p_cond, pthread_mutex_t * p_lock) {if(pthread_cond_wait(p_cond, p_lock) != 0) err_quit("An error occurred during cond wait.");}
void Signal(pthread_cond_t * p_cond) {if(pthread_cond_signal(p_cond) != 0) err_quit("An error occurred during a condition singal.");}
void Broadcast(pthread_cond_t * p_cond) {if(pthread_cond_broadcast(p_cond) != 0) err_quit("An error occurred during a brodcast.");}

