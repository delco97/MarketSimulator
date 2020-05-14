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
#include <configFileParser.h>

/**
 * @brief Explain how to correctly use the program and exit.
 * 
 * @param p_argv parameters passed to the program.
 */
void useInfo(char * p_argv[]){
	fprintf(stderr, "Wrong use. See the expected call:\n");
	fprintf(stderr, "	%s <config_file> <log_file>\n", p_argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char * argv[]) {
	FILE * f_log = NULL;
	char userChoice;
	
	if(argc != 3){//Wrong use
		useInfo(argv);
	}
	//Check the log file path
	f_log = fopen(argv[2], "r");
	if( f_log != NULL ) {
    	printf("Log file %s already exist. If you want to proceed it will be overwritten.", argv[2]);
		do{
			printf("\nDo you want to proceed?[y/n] ");
			userChoice = getchar();
		}while(userChoice != 'y' && userChoice != 'n');
		if(userChoice == 'n'){
			fclose(f_log);
			exit(0);
		}
		fclose(f_log);
	}

	//Read configurations
	printf("Reading configuration file %s ...\n", argv[1]);
	if(parseConfigFile(argv[1]) != 1) {
		err_quit("Impossible to setup the market, check the configuration file and try again.");
	}
	printf("Done!\n");
	printf("**Welcome to Market simulator**\n");

	return 0;
}