/**
 * @file main.c
 * @brief	This is the entry point of the application.
 * 			It is used to setup the environment using the config file passed
 * 			as parameter.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <utilities.h>
#include <SQueue.h>
#include <Config.h>
#include <TMarket.h>

volatile sig_atomic_t sig_hup=0; /**< SIGHUP signal indicator */
volatile sig_atomic_t sig_quit=0; /**< SIGQUIT signal indicator */
/**
 * @brief Data struct used to pass paramters to signal handler thread
 * 
 */
typedef struct _input_handler_par_t {
	Market * m; /**<reference to market interested into getting notified about signals.*/
	sigset_t * set;	/**< set of signal handled*/
} input_handler_par_t;

/**
 * @brief Signal handler thread's main function.
 * 
 * It handles the following signals:
 *  - SIGQUIT: set sig_quit = 1 and notify Market thread that will start a fast-closure.
 * 	- SIHUP: set sig_hup = 1 and notify Market thread that will start a gracefull-closure.
 * @param p_arg this argument is expected to be a input_handler_par_t *
 * @return void* 
 */
static void * sigHandler(void * p_arg) {
	sigset_t * set = ((input_handler_par_t *) p_arg)->set;
	Market * m = ((input_handler_par_t *) p_arg)->m;
    int sig;

    while (1) {
        if (sigwait(set, &sig) != 0) ERR_QUIT("sigwait");
        switch (sig) {
			case SIGQUIT:
				printf("Received signal SIGQUIT.\n");
				sig_quit = 1;
				Signal(&m->cv_MarketNews);
				return (void *) NULL;			
				break;
			case SIGHUP:
				printf("Received signal SIGHUP.\n");
				sig_hup = 1;
				Signal(&m->cv_MarketNews);
				return (void *) NULL;
				break;       
			default:
				ERR_MSG("Received unknown signal: %d!\n", sig);
				break;
        }
    }	
}

/**
 * @brief Print a message on stdout to explain how to correctly use the program.
 * 
 * @param p_argv parameters passed to the program.
 */
static void useInfo(char * p_argv[]){
	fprintf(stderr, "See the expected call:\n");
	fprintf(stderr, "	%s <config_file> <log_file>\n", p_argv[0]);
}

int main(int argc, char * argv[]) {
	Market * m = NULL;
	pthread_t thSigHandler;
	input_handler_par_t in;
	sigset_t set;	

	DEBUG_PRINT("PID: %d\n", getpid());

	if(argc != 3){//Wrong use
		printf("Wrong use.");
		useInfo(argv);
		ERR_QUIT("Exit...");
	}

	g_seed = time(NULL); 	//Init seed (each thread has its own seed thanks to keyword _Thread_local)
							//used to produce random numbers with rand_r.

	//Setup signal handler thread in order to block SIGHUP and SIGHUP signals.
	//Other threads created by main() thread will inherit a copy of its signal mask, so they
	//won't receive SIGHUP and SIGHUP as main(), because these signal will handled by the signal handler thread.
	if(sigemptyset(&set) == -1) ERR_QUIT("impossible to set mask.");
	if(sigaddset(&set, SIGHUP) == -1) ERR_QUIT("impossible to set mask. (2)");
	if(sigaddset(&set, SIGQUIT) == -1) ERR_QUIT("impossible to set mask. (3)");
	if(pthread_sigmask(SIG_BLOCK, &set, NULL)==-1) ERR_QUIT("impossible to set mask (4)");	

	//Try to init market
	if((m = Market_init(argv[1], argv[2])) == NULL)
		ERR_QUIT("An error occurred during market initialization. Exit...");

	//Market is correctly initialized
	if(Market_startThread(m) != 0)
		ERR_QUIT("An error occurred during market startup (1). Exit...");

	//Start signal handler thread
	in.m = m;
	in.set = &set;
	if(pthread_create(&thSigHandler, NULL, sigHandler, &in) != 0)
		ERR_QUIT("impossible to execute signal handler thread.");

	//Wait Market
	if(Market_joinThread(m) != 0)
		ERR_QUIT("An error occurred during market startup (2). Exit...");
	
	//Deallocate memory used by Market
	if(Market_delete(m) != 1)
		ERR_QUIT( "An error occurred during market closing. Exit...");
	
	//Wait signal handler thread
	if(pthread_join(thSigHandler, NULL) != 0)
		ERR_QUIT("pthread_join: thSigHandler");

	printf("Market closed.\n");

	return 0;
}