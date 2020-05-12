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

