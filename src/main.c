/**
 * @file main.c
 * @brief	This is the entry point of the application.
 * 			It is used to setup the environment using the config file passed
 * 			as parameter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <utilities.h>
#include <SQueue.h>
#include <Config.h>
#include <TMarket.h>
/**
 * @brief Explain how to correctly use the program.
 * 
 * @param p_argv parameters passed to the program.
 */
void useInfo(char * p_argv[]){
	fprintf(stderr, "See the expected call:\n");
	fprintf(stderr, "	%s <config_file> <log_file>\n", p_argv[0]);
}

int main(int argc, char * argv[]) {
	Market * m;

	if(argc != 3){//Wrong use
		printf("Wrong use.");
		useInfo(argv);
		err_exit(EXIT_FAILURE, "Exit...");
	}

	//Try to init market
	if((m = Market_init(argv[1], argv[2])) == NULL)
		err_exit(EXIT_FAILURE, "An error occurred during market initialization. Exit...");
	
	//Market is correctly initialized
	if(Market_startThread(m) != 0)
		err_exit(EXIT_FAILURE, "An error occurred during market startup (1). Exit...");
	
	//Wait Market
	if(Market_joinThread(m) != 0)
		err_exit(EXIT_FAILURE, "An error occurred during market startup (2). Exit...");

	return 0;
}