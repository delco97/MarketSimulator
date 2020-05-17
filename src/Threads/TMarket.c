#include <TMarket.h>
#include <TUser.h>
#include <utilities.h>
#include <Config.h>
#include <stdlib.h>
#include <stdio.h>

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
	if(Config_parseLong(p_x, str_aux) != 1){
		err_msg("The property %s is defined but it has a wrong value format.\n");
		return -2;
	}
	return 1;
}
/**
 * @brief Check if constraint is satisfied. If it is not, display a warning message.
 * @param p_check is the result of the check.
 * @param p_contraint is the constraint label to use in case of error.
 * @return int: result code:
 * 1: good
 * -1: constraint not satisfied
 */
static int checkContraint(int p_check, const char * p_contraint){
	if(!p_check){
		err_msg("The following constrait is not satisfied: %s\n", p_contraint);
		return -1;
	}
	return 1;
}

static void pDeallocUser(void * p_arg){
	User * u = (User *) p_arg;
	User_delete(u);
}

/**
 * @brief Create a new Market object.
 * 
 * @param p_conf configuration file used to init the market.
 * @param p_log path to log file which will contain simulation results.
 * @return Market* pointer to new market allocated, NULL if a probelm occurred during allocation. 
 */
Market * Market_init(const char * p_conf, const char * p_log){
	Market * m = NULL;
	FILE * f_log = NULL;
	FILE * f_conf = NULL;
	int res = 1;
	char userChoice;
	
	//Check the log file path
	f_log = fopen(p_log, "r");
	if( f_log != NULL ) {
    	printf("Log file %s already exist. If you want to proceed it will be overwritten.", p_log);
		do{
			printf("\nDo you want to proceed?[y/n] ");
			userChoice = getchar();
		}while(userChoice != 'y' && userChoice != 'n');
		if(userChoice == 'n') goto err;
		fclose(f_log);
		f_log = NULL;
	}

	//Check the config file path
	f_conf = fopen(p_conf, "r");
	if( f_conf == NULL ) {
		err_ret("Unable to open configuration file %s. Check the path and try again.");
		goto err;
	}

	//Read configurations
	printf("Reading configuration file %s ...\n", p_conf);
	if(Config_checkFile(f_conf) != 1) {
		err_msg("Impossible to setup the market, because there are some error in the config file.\nFix them and try again.");
		goto err;
	}
	//Try to read from configuration file
	if((m = malloc(sizeof(Market))) == NULL){
		err_ret("An error occurred during memory allocation.");
		goto err;
	}
	printf("Checking if all configuration items required are defined...\n");
	//Format check
	res = pGetLong(f_conf, "K", &m->K) != 1 ? 0:res;
	res = pGetLong(f_conf, "KS", &m->KS) != 1 ? 0:res;
	res = pGetLong(f_conf, "C", &m->C) != 1 ? 0:res;
	res = pGetLong(f_conf, "E", &m->E) != 1 ? 0:res;
	res = pGetLong(f_conf, "T", &m->T) != 1 ? 0:res;
	res = pGetLong(f_conf, "P", &m->P) != 1 ? 0:res;
	res = pGetLong(f_conf, "S", &m->S) != 1 ? 0:res;
	res = pGetLong(f_conf, "S1", &m->S1) != 1 ? 0:res;
	res = pGetLong(f_conf, "S2", &m->S2) != 1 ? 0:res;
	res = pGetLong(f_conf, "NP", &m->NP) != 1 ? 0:res;

	fclose(f_conf);
	f_conf = NULL;

	if(res != 1){
		printf("Some configuration items are missing or have wrong value format. Edit the configuration file and try again.\n");
		goto err;
	}
	printf("All configuration items required are correctly defined.\n");
	printf("Checking if all constraints overs configuration items are satisfied...\n");
	//Check values constraints
	res = checkContraint(m->K > 0, "{K>0}") != 1 ? 0:res;
	res = checkContraint(m->KS > 0 && m->KS <= m->K, "{0<KS<=K}") != 1 ? 0:res;
	res = checkContraint(m->C > 1, "{C>1}") != 1 ? 0:res;			
	res = checkContraint(m->E > 0 && m->E < m->C, "{0<E<C}") != 1 ? 0:res;
	res = checkContraint(m->T > 10, "{T>10}") != 1 ? 0:res;
	res = checkContraint(m->P > 0, "{P>0}") != 1 ? 0:res;
	res = checkContraint(m->S > 0, "{S>0}") != 1 ? 0:res;
	res = checkContraint(m->S1 > 0, "{S1>0}") != 1 ? 0:res;
	res = checkContraint(m->S2 > 0, "{S2>0}") != 1 ? 0:res;
	res = checkContraint(m->NP > 0, "{NP>0}") != 1 ? 0:res;
	
	if(res != 1) {
		printf("Some constraint are not satisfied. Edit the configuration file and try again.\n");
		goto err;
	}
	printf("All constraints are satisfied.\n");


	//Director init
	if((m->director = Director_init(m)) == NULL){
		err_msg("An error occurred during director creation. Impossible to setup the market. ");
		goto err;
	}
	
	//Queues init
	if((m->usersShopping = SQueue_init(-1)) == NULL || (m->usersExit = SQueue_init(-1)) == NULL){
		err_msg("An error occurred during queues creation. Impossible to setup the market.");
		goto err;
	}

	//Init lock system
	if (pthread_mutex_init(&(m->lock), NULL) != 0) {
		err_msg("An error occurred during locking system initialization. Impossible to setup the market.");
		goto err;
	}

	printf("Done!\n");
	printf("**Welcome to Market simulator**\n");
	return m;
err:
	if(f_conf != NULL) fclose(f_conf);
	if(f_log != NULL) fclose(f_log);
	if(m != NULL){
		if(m->director != NULL) Director_delete(m->director);
		if(m->usersShopping != NULL) SQueue_deleteQueue(m->usersShopping, NULL);
		if(m->usersExit != NULL) SQueue_deleteQueue(m->usersExit, NULL);
		pthread_mutex_destroy(&m->lock);
		free(m);
	}
	return NULL;
}

/**
 * @brief Start Market thread.
 *        The behaviour is undefined if p_u has not been previously initialized with #Market_init.
 * 
 * @param p_u Requirements: p_m != NULL and must refer to a Market object created with #Market_init. Target Market.
 * @return int: result pf pthread_create call
 */
int Market_startThread(Market * p_m){
	return pthread_create(&p_m->thread, NULL, Market_main, p_m);
}

/**
 * @brief Join the market thread
 * 
 * @param p_m Requirements: p_m != NULL and must refer to a User object created with #Market_init. Target Market.
 * @return int result of pthread_join
 */
int Market_joinThread(Market * p_m){
	return pthread_join(p_m->thread, NULL);
}

/**
 * @brief Dealloc a Market object.
 * 
 * @warning This function should be called by only one thread when no other thread is working on p_m object.
 *          Typically the main thread call this function after all slave threads have terminated.
 * 
 * @param p_m Requirements: p_m != NULL and must refer to a User object created with #Market_init. Target Market.
 * @return int: result code:
 *  1: p_m != NULL and the deallocation proceed witout errors. 
 *  -1: p_m == NULL
 */
int Market_delete(Market * p_m) {
    if(p_m == NULL) return -1; 
	Director_delete(p_m->director);
	SQueue_deleteQueue(p_m->usersShopping, pDeallocUser);
	SQueue_deleteQueue(p_m->usersExit, pDeallocUser);
	pthread_mutex_destroy(&p_m->lock);
    free(p_m);
    return 1;
}

void * Market_main(void * arg){

    return (void *)NULL;
}



