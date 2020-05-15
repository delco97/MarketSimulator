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
	FILE * f_conf = NULL;
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

	//Check the config file path
	f_conf = fopen(argv[1], "r");
	if( f_conf == NULL ) {
		err_sys("Unable to open configuration file %s. Check the path and try again.");
	}

	//Read configurations
	printf("Check configuration file %s ...\n", argv[1]);
	if(Config_checkFile(f_conf) != 1) {
		err_quit("Impossible to setup the market, because there are some error in the config file.\nFix them and try again.");
	}
	printf("Done!\n");

	printf("**Welcome to Market simulator**\n");

	return 0;
}