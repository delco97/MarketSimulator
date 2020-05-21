/**
 * @file main.c
 * @brief	This is the entry point of the application.
 * 			It is used to setup the environment using the config file passed
 * 			as parameter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <utilities.h>
#include <SQueue.h>
#include <Config.h>
#include <TMarket.h>
#include <signal.h>

volatile sig_atomic_t sig_hup=0; /**< SIGHUP signal indicator */
volatile sig_atomic_t sig_quit=0; /**< SIGQUIT signal indicator */

typedef struct _input_handler_par_t {
	Market * m;
	sigset_t * set;
} input_handler_par_t;

static void * sigHandler(void * p_arg) {
	sigset_t * set = ((input_handler_par_t *) p_arg)->set;
	Market * m = ((input_handler_par_t *) p_arg)->m;
    int sig;

    while (1) {
        if (sigwait(set, &sig) != 0) err_quit("sigwait");
        switch (sig) {
			case SIGQUIT:
				printf("Received signal SIGQUIT.\n");
				Lock(&m->lock);
				sig_quit = 1;
				Broadcast(&m->cv_MarketNews);
				Unlock(&m->lock);
				return (void *) NULL;			
				break;
			case SIGHUP:
				printf("Received signal SIGHUP.\n");
				Lock(&m->lock);
				sig_hup = 1;
				Broadcast(&m->cv_MarketNews);
				Unlock(&m->lock);
				return (void *) NULL;
				break;       
			default:
				err_msg("Received unknown signal: %d!\n", sig);
				break;
        }
    }	
}

/**
 * @brief Explain how to correctly use the program.
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
	printf("PID: %d\n", getpid());
	if(argc != 3){//Wrong use
		printf("Wrong use.");
		useInfo(argv);
		err_exit(EXIT_FAILURE, "Exit...");
	}
    
	//Setup signal handler thread
    //Block SIGHUP and SIGQUIT; other threads created by main()
	//will inherit a copy of the signal mask.	
	if(sigemptyset(&set) == -1) err_quit("impossible to set mask. (1)");
	if(sigaddset(&set, SIGHUP) == -1) err_quit("impossible to set mask. (2)");
	if(sigaddset(&set, SIGQUIT) == -1) err_quit("impossible to set mask. (3)");
	if(pthread_sigmask(SIG_BLOCK, &set, NULL)==-1) err_quit("impossible to set mask (4)");	

	//Try to init market
	if((m = Market_init(argv[1], argv[2])) == NULL)
		err_quit("An error occurred during market initialization. Exit...");

	//Market is correctly initialized
	if(Market_startThread(m) != 0)
		err_quit("An error occurred during market startup (1). Exit...");

	//Start signal handler thread
	in.m = m;
	in.set = &set;
	if(pthread_create(&thSigHandler, NULL, sigHandler, &in) != 0)
		err_quit("impossible to execute signal handler thread.");

	//Wait Market
	if(Market_joinThread(m) != 0)
		err_quit("An error occurred during market startup (2). Exit...");

	if(Market_delete(m) != 1)
		err_quit( "An error occurred during market closing. Exit...");
	
	//Wait signal handler thread
	if(pthread_join(thSigHandler, NULL) != 0)
		err_quit("pthread_join: thSigHandler");

	printf("Market closed\n");

	return 0;
}