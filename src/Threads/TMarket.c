#include <TMarket.h>
#include <utilities.h>
#include <Config.h>

/**
 * @file TMarket.c
 * @brief  Implementation of Market.
 */

//Private functions
/**
 * @brief Try to get value associated to p_key from the config file f and to parse it as long
 * 
 * @param f config file
 * @param p_key key to search
 * @param p_x where the value will be placed
 * @return int: result code:
 * 1: p_x contain value associated to p_key in the config file
 * -1: property is not defined
 * -2: property is defined but it can't be parsed as long
 */
static int pGetLong(FILE * f, const char * p_key, long * p_x){
	char str_aux[MAX_DIM_STR_CONF]; //Used to get string value from config file
	
	if(Config_getValue(f, p_key, str_aux) != 1){
		err_msg("The property %s is not defined.");
		return -1;
	}
	if(!Config_parseLong(p_x, str_aux) != 1){
		err_msg("The property %s is defined but it has a wrong value format.\n");
		return -2;
	}
	return 1;
}


/**
 * @brief Create a new Market object.
 * 
 * @param p_conf configuration file used to init the market.
 * @param p_log path to log file which will contain simulation results.
 * @return Market* pointer to new market allocated, NULL if a probelm occurred during allocation. 
 */
Market * User_init(const char * p_conf, const char * p_log){
	Market * m = NULL;
	FILE * f_log = NULL;
	FILE * f_conf = NULL;
	char userChoice;
	
	//Check the log file path
	f_log = fopen(p_log, "r");
	if( f_log != NULL ) {
    	printf("Log file %s already exist. If you want to proceed it will be overwritten.", p_log);
		do{
			printf("\nDo you want to proceed?[y/n] ");
			userChoice = getchar();
		}while(userChoice != 'y' && userChoice != 'n');
		if(userChoice == 'n'){
			fclose(f_log);
			return NULL;
		}
		fclose(f_log);
	}

	//Check the config file path
	f_conf = fopen(p_conf, "r");
	if( f_conf == NULL ) {
		err_msg("Unable to open configuration file %s. Check the path and try again.");
		return NULL;
	}

	//Read configurations
	printf("Reading configuration file %s ...\n", p_conf);
	if(Config_checkFile(f_conf) != 1) {
		err_cont("Impossible to setup the market, because there are some error in the config file.\nFix them and try again.");
		return NULL;
	}
	//Try to read from configuration file
	if((m = malloc(sizeof(Market))) == NULL)
		err_sys("An error occurred during memory allocation.");

	if(	!pGetLong(f_conf, "K", &m->K) || 
		!pGetLong(f_conf, "KS", &m->K) ||
		!pGetLong(f_conf, "C", &m->K) ||
		!pGetLong(f_conf, "E", &m->K) ||
		!pGetLong(f_conf, "T", &m->K) ||
		!pGetLong(f_conf, "P", &m->K) ||
		!pGetLong(f_conf, "S", &m->K) ||
		!pGetLong(f_conf, "S1", &m->K) ||
		!pGetLong(f_conf, "S2", &m->K) ||
		!pGetLong(f_conf, "NP", &m->K)){
			free(m);
			return NULL;
		}
	//Good parsing, check constraints
	if(m->k < 0)

	printf("Done!\n");




	printf("**Welcome to Market simulator**\n");
}


