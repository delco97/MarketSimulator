#include <TMarket.h>
#include <TUser.h>
#include <TCashDesk.h>
#include <Config.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <PayArea.h>
#include <utilities.h>

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
		ERR_MSG("The property %s is not defined.", p_key);
		return -1;
	}
	if(Config_parseLong(p_x, str_aux) != 1){
		ERR_MSG("The property %s is defined but it has a wrong value format.\n", p_key);
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
static int pCheckContraint(int p_check, const char * p_contraint){
	if(!p_check){
		ERR_MSG("The following constrait is not satisfied: %s\n", p_contraint);
		return -1;
	}
	return 1;
}

static void pDeallocUser(void * p_arg){
	User * u = (User *) p_arg;
	User_delete(u);
}


/**
 * @brief Move user p_u from shopping to a open cashdesk.
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user who is moving
 */
void Market_FromShoppingToPay(Market * p_m, User * p_u) {
	//Remove user from shopping
	if( SQueue_remove(p_m->usersShopping, p_u, User_compare) != 1)
		ERR_QUIT("Impossible to find User %d in shopping area.", p_u->id);
	//Move user to a random open cash desk
	PayArea_addUser(p_m->payArea, p_u);
}

/**
 * @brief Move user p_u from shopping directly to exit queue
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user who is moving
 */
void Market_FromShoppingToExit(Market * p_m, User * p_u) {
	struct timespec cur;
	//Remove user from shopping
	if( SQueue_remove(p_m->usersShopping, p_u, User_compare) != 1)
		ERR_QUIT("Impossible to find User %d in shopping area.", p_u->id);
	cur = getCurrentTime();
	p_u->tQueueStart = cur;
	p_u->tMarketExit = cur;
	p_u->products = 0;
	if(SQueue_push(p_m->usersExit, p_u) != 1)
		ERR_QUIT("Impossible to move User %d in exit queue.", p_u->id);
	Signal(&p_m->cv_MarketNews);
}

/**
 * @brief Move user p_u from shopping are to a authorization queue
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user who is moving
 */
void Market_FromShoppingToAuth(Market * p_m, User * p_u) {
	//Remove user from shopping
	if( SQueue_remove(p_m->usersShopping, p_u, User_compare) != 1)
		ERR_QUIT("Impossible to find User %d in shopping area.", p_u->id);
	p_u->tQueueStart = getCurrentTime();
	p_u->queueChanges++;
	if(SQueue_push(p_m->usersAuthQueue, p_u) != 1)
		ERR_QUIT("Impossible to move User %d in authorization queue.", p_u->id);
	Signal(&p_m->director->cv_Director_AuthNews);
}

/**
 * @brief Move users p_u to the exit queue
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user to move
 */
void Market_moveToExit(Market * p_m, User * p_u){
	p_u->tMarketExit = getCurrentTime();
	if(SQueue_push(p_m->usersExit, p_u) != 1)
		ERR_QUIT("Impossible to move User %d in exit queue.", p_u->id);
	Signal(&p_m->cv_MarketNews);
}

/**
 * @brief Log data to statistics file.
 * 
 * @param p_m Requirements: p_m != NULL and must refer to a Market object created with #Market_init. Target Market.
 * @param p_data string to write into file
 */
void Market_log(Market * p_m, char * p_data) {
	Lock(&p_m->lock_Logfile);
	fprintf(p_m->f_log, "%s\n", p_data);
	Unlock(&p_m->lock_Logfile);
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
	int isLockInit = 0;

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
	//Open log file for writing
	f_log = fopen(p_log, "w");
	if( f_log == NULL ) {
		ERR_SYS_MSG("Unable to open log file %s. Check the path and try again.", p_log);
		goto err;
	}

	//Check the config file path
	f_conf = fopen(p_conf, "r");
	if( f_conf == NULL ) {
		ERR_SYS_MSG("Unable to open configuration file %s. Check the path and try again.", p_conf);
		goto err;
	}

	//Read configurations
	printf("Reading configuration file %s ...\n", p_conf);
	if(Config_checkFile(f_conf) != 1) {
		ERR_MSG("Impossible to setup the market, because there are some error in the config file.\nFix them and try again.");
		goto err;
	}
	//Try to read from configuration file
	if((m = malloc(sizeof(Market))) == NULL){
		ERR_SYS_MSG("An error occurred during memory allocation.");
		goto err;
	}
	m->f_log = f_log;
	//Default values
	m->director = NULL;
	m->usersShopping = NULL;
	m->usersExit = NULL;
	m->usersAuthQueue = NULL;
	m->payArea = NULL;
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
	res = pGetLong(f_conf, "TD", &m->TD) != 1 ? 0:res;

	fclose(f_conf);
	f_conf = NULL;

	if(res != 1){
		printf("Some configuration items are missing or have wrong value format. Edit the configuration file and try again.\n");
		goto err;
	}
	printf("All configuration items required are correctly defined.\n");
	printf("Checking if all constraints overs configuration items are satisfied...\n");
	//Check values constraints
	res = pCheckContraint(m->K > 0, "{K>=1}") != 1 ? 0:res;
	res = pCheckContraint(m->KS > 0 && m->KS <= m->K, "{0<KS<=K}") != 1 ? 0:res;
	res = pCheckContraint(m->C >= 1, "{C>=1}") != 1 ? 0:res;			
	res = pCheckContraint(m->E > 0 && m->E <= m->C, "{0<E<=C}") != 1 ? 0:res;
	res = pCheckContraint(m->T > 10, "{T>10}") != 1 ? 0:res;
	res = pCheckContraint(m->P > 0, "{P>0}") != 1 ? 0:res;
	res = pCheckContraint(m->S > 0, "{S>0}") != 1 ? 0:res;
	res = pCheckContraint(m->S1 > 0 && m->S1 <= m->K, "{0<S1<=K}") != 1 ? 0:res;
	res = pCheckContraint(m->S2 > 0 && m->S2 <= m->C, "{0<S2<=C}") != 1 ? 0:res;
	res = pCheckContraint(m->NP > 0, "{NP>0}") != 1 ? 0:res;
	res = pCheckContraint(m->TD > 0, "{TD>0}") != 1 ? 0:res;
	
	if(res != 1) {
		printf("Some constraint are not satisfied. Edit the configuration file and try again.\n");
		goto err;
	}
	printf("All constraints are satisfied.\n");


	//Director init
	if((m->director = Director_init(m)) == NULL){
		ERR_MSG("An error occurred during director creation. Impossible to setup the market. ");
		goto err;
	}
	
	//Queues init
	if(	(m->usersShopping = SQueue_init(-1)) == NULL || 
		(m->usersExit = SQueue_init(-1)) == NULL ||
		(m->usersAuthQueue = SQueue_init(-1)) == NULL){
		ERR_MSG("An error occurred during queues creation. Impossible to setup the market.");
		goto err;
	}

	//Init payArea
	if( (m->payArea = PayArea_init(m, m->K, m->KS)) == NULL) {
		ERR_MSG("An error occurred during pay area creation. Impossible to setup the market. ");
		goto err;
	}

	//Init lock system
	if (pthread_mutex_init(&(m->lock), NULL) != 0 ||
		pthread_cond_init(&m->cv_MarketNews, NULL) != 0 ||
		pthread_mutex_init(&m->lock_Logfile, NULL) != 0) {
		ERR_MSG("An error occurred during locking system initialization. Impossible to setup the market.");
		goto err;
	}
	isLockInit = 1;

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
		if(m->usersAuthQueue != NULL) SQueue_deleteQueue(m->usersAuthQueue, NULL);
		if(m->payArea != NULL) PayArea_delete(m->payArea);
		if(isLockInit){
			pthread_mutex_destroy(&m->lock);
			pthread_cond_destroy(&m->cv_MarketNews);
			pthread_mutex_destroy(&m->lock_Logfile);
		}
		free(m);
	}
	return NULL;
}

/**
 * @brief Start Market thread.
 *        The behaviour is undefined if p_u has not been previously initialized with #Market_init.
 * 
 * @param p_m Requirements: p_m != NULL and must refer to a Market object created with #Market_init. Target Market.
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
	SQueue_deleteQueue(p_m->usersAuthQueue, pDeallocUser);
	PayArea_delete(p_m->payArea);
	pthread_mutex_destroy(&p_m->lock);
	pthread_cond_destroy(&p_m->cv_MarketNews);
	pthread_mutex_destroy(&p_m->lock_Logfile);
    free(p_m);
    return 1;
}

void Market_Lock(Market * p_m) {Lock(&p_m->lock);}
void Market_Unlock(Market * p_m) {Unlock(&p_m->lock);}

/**
 * @brief Check if the market is currently empty
 * 
 * @param p_m Requirements: p_m != NULL and must refer to a User object created with #Market_init. Target Market.
 * @return int: resul code:
 * 1: p_m is empty
 * 0: p_m is not empty
 */
int Market_isEmpty(Market * p_m){
	int res_fun = 1;
	res_fun = res_fun!=1 || SQueue_isEmpty(p_m->usersShopping)!=1 ? 0:res_fun;
	res_fun = res_fun!=1 || SQueue_isEmpty(p_m->usersAuthQueue)!=1 ? 0:res_fun;
	//Check if all cash desk are empty
	res_fun = res_fun!=1 || PayArea_isEmpty(p_m->payArea)!=1 ? 0:res_fun;
	res_fun = res_fun!=1 || SQueue_isEmpty(p_m->usersExit)!=1 ? 0:res_fun;
	if(res_fun == 0) DEBUG_PRINT("1) Market non vuoto: %d (shopping), %d (auth), %d (exit), %d (pay area is empty)\n", SQueue_dim(p_m->usersShopping), SQueue_dim(p_m->usersAuthQueue), SQueue_dim(p_m->usersExit), PayArea_isEmpty(p_m->payArea));
	if(res_fun == 0) DEBUG_PRINT("2) Market non vuoto: %d (shopping), %d (auth), %d (exit), %d (pay area is empty)\n", SQueue_isEmpty(p_m->usersShopping), SQueue_isEmpty(p_m->usersAuthQueue), SQueue_isEmpty(p_m->usersExit), PayArea_isEmpty(p_m->payArea));
	return res_fun;
}

/**
 * @brief Entry point for the Market thread.
 * 
 * Function to use on Market thread creation. This
 * function handle the Market data structure passed as argument in the following way:
 *  1. Start Cashdesks and Director threads.
 * 	2. Create C users and put theme in shopping area
 *  2. When E users left the market they are inserted again in shoppig area.
 * @param p_arg argument passed to the Market thread. Market type expected.
 * @return void* 
 */
void * Market_main(void * p_arg){
	Market * m = (Market *) p_arg;
	User * u_aux = NULL;
	int numExit = 0; //count users exit until E is reached
	void * data;
	SQueue * newGroup = NULL;
	int removedUsers = 0;

	if((newGroup = SQueue_init(-1)) == NULL)
		ERR_QUIT("[Market]: An error occurred during market startup. (newGroup init failed)");
	
	//Start CashDesks Threads
	PayArea_startDeskThreads(m->payArea);

	//Create and add C users in shopping area
	//Lock(&m->lock);
	for(int i = 0; i < m->C; i++){
		if((u_aux = User_init(getRandom(0, m->P), getRandom(10, m->T), m)) == NULL)
			ERR_QUIT("[Market]: An error occurred during market startup. (User init failed)");
		SQueue_push(m->usersShopping, u_aux);
		if(User_startThread(u_aux) != 0)
			ERR_QUIT("[Market]: An error occurred during market startup. (User startThread failed)");		
	}
	//Unlock(&m->lock);

	//Start Director thread
	if(Director_startThread(m->director) != 0)
		ERR_QUIT("[Market]: An error occurred during desk thread start. (CashDesk startThread failed)");


	//Wait E users exits
	while (1) {
		//Wait a signal or new user in exit queue to proceed
		Lock(&m->lock);
		while (sig_hup != 1 && sig_quit != 1 && SQueue_isEmpty(m->usersExit)==1) 
			pthread_cond_wait(&m->cv_MarketNews, &m->lock);
		Unlock(&m->lock);

		if(sig_hup == 1 || sig_quit == 1) {
			printf("Market is closing...\n");
			//When SIGHUP or SIQQUIT occurs no new users are allowed inside the market and
			//all the users inside are waited.
			//Singal closure to all threads and wait them
			PayArea_Signal(m->payArea);
			printf("Cashdesks termination...\n");
			PayArea_joinDeskThreads(m->payArea);
			printf("Wait director termination...\n");
			Signal(&m->director->cv_Director_AuthNews);
			Signal(&m->director->cv_Director_DesksNews);			
			if(Director_joinThread(m->director)!=0) ERR_QUIT("An error occurred during director thread join.");			
			//Remove all users from exit queue
			printf("Removing users from exit queue..\n");
			//Move all users in newGroup into exit 
			while(SQueue_pop(newGroup, &data) != -2) {
				u_aux = (User *) data;
				SQueue_push(m->usersExit, u_aux);
			}
			//Delete all users
			while(SQueue_pop(m->usersExit, &data) == 1) {		
				u_aux = (User *) data;
				User_log(u_aux);
				u_aux->state = USR_QUIT;
				Signal(&u_aux->cv_UserNews);
				if(User_joinThread(u_aux) != 0)
					ERR_QUIT("An error occurred joining User %d thread.", u_aux->id);
				User_delete(u_aux);
				removedUsers++;
				printf("[Market]: Users removed: %d\n", removedUsers);
			}	
			//Log all cashdesks data
			DEBUG_PRINT("Market_isEmpty: %d\n", Market_isEmpty(m));
			printf("Log all cash desks statistics..\n");
			for(int i = 0; i < m->K; i++) 
				CashDesk_log(m->payArea->desks[i]);
			
			break;					
		}
		//Market is not closing
		if(SQueue_pop(m->usersExit, &data) == 1) {
			u_aux = (User *) data;		
			numExit++;
			//Log user info.
			User_log(u_aux);	
			//Reset user structure for next reuse
			User_reset(u_aux, getRandom(0, m->P), getRandom(10, m->T), m);
			SQueue_push(newGroup, u_aux);
			if(numExit == m->E) {//E numExits
				//Move all users in newGroup into shopping area
				while(SQueue_pop(newGroup, &data) != -2) {
					u_aux = (User *) data;
					SQueue_push(m->usersShopping, u_aux);
					u_aux->state = USR_READY;
					Signal(&u_aux->cv_UserNews);
				}
				numExit = 0;
			}
		}
	}

	SQueue_deleteQueue(newGroup, pDeallocUser);
		
    return (void *)NULL;
}

