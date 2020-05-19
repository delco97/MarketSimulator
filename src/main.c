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

static void sigHandler(int p_sig) {
	switch (p_sig){
		case SIGHUP:
			if(sig_quit ==0){
				sig_hup=1;
				write(1,"Received signal SIGHUP\n",24);
			}else write(1,"Signal SIGHUP ignored\n",23);
			
			break;
		case SIGQUIT:
			if(sig_hup == 0){
				sig_quit=1;
				write(1,"Received signal SIGQUIT\n",25);
			} else write(1,"Signal SIGQUIT ignored\n",24);
			break;
		default:
			write(2,"Received unexpected signal\n",28);
			_exit(EXIT_FAILURE);
			break;
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

/**
 * @brief Setup signal handlers managed by the application.
 */
static void setupHandlers() {
    sigset_t set;
	struct sigaction s;
	//Mask all signals untill all custom
	//handlers have been installed
	if(sigfillset(&set)==-1) err_quit("impossible to set mask");
	if(pthread_sigmask(SIG_SETMASK, &set, NULL)==-1) err_quit("pthread_sigmask");	
	memset(&s,0,sizeof(s));
	//Add custom handlers
    s.sa_handler=sigHandler;
    if (sigaction(SIGHUP,&s,NULL)==-1) err_quit("impossible to setup signal handler (1).");
    if (sigaction(SIGQUIT,&s,NULL)==-1) err_quit("impossible to setup signal handler (2).");
	//Remove the mask
	if(sigemptyset(&set)==-1) err_quit("impossible to remove mask.");
	if(pthread_sigmask(SIG_SETMASK,&set,NULL)==-1) err_quit("impossible to remove mask.");
}

int main(int argc, char * argv[]) {
	Market * m;
	printf("PID: %d\n", getpid());
	if(argc != 3){//Wrong use
		printf("Wrong use.");
		useInfo(argv);
		err_exit(EXIT_FAILURE, "Exit...");
	}

	//Set signal handler
	setupHandlers();

	//Try to init market
	if((m = Market_init(argv[1], argv[2])) == NULL)
		err_quit("An error occurred during market initialization. Exit...");
	
	//Market is correctly initialized
	if(Market_startThread(m) != 0)
		err_quit("An error occurred during market startup (1). Exit...");
	
	//Wait Market
	if(Market_joinThread(m) != 0)
		err_quit("An error occurred during market startup (2). Exit...");

	if(Market_delete(m) != 1)
		err_quit( "An error occurred during market closing. Exit...");

	printf("Market closed\n");

	return 0;
}