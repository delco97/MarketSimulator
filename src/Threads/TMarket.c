#include <TMarket.h>
#include <TUser.h>
#include <TCashDesk.h>
#include <utilities.h>
#include <Config.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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
static int pCheckContraint(int p_check, const char * p_contraint){
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

unsigned int * Market_getSeed(Market * p_m) {return &p_m->seed;}
long Market_getK(Market * p_m) {return p_m->K;}
long Market_getKS(Market * p_m) {return p_m->KS;}
long Market_getC(Market * p_m) {return p_m->C;}
long Market_getE(Market * p_m) {return p_m->E;}
long Market_getT(Market * p_m) {return p_m->T;}
long Market_getP(Market * p_m) {return p_m->P;}
long Market_getS(Market * p_m) {return p_m->S;}
long Market_getS1(Market * p_m) {return p_m->S1;}
long Market_getS2(Market * p_m) {return p_m->S2;}
long Market_getNP(Market * p_m) {return p_m->NP;}
Director * Market_getDirector(Market * p_m) {return p_m->director;}
SQueue * Market_getUsersShopping(Market * p_m) {return p_m->usersShopping;}
SQueue * Market_getUsersExit(Market * p_m) {return p_m->usersExit;}
SQueue * Market_getUsersAuth(Market * p_m) {return p_m->usersAuthQueue;}
CashDesk * Market_getDesks(Market * p_m) {return p_m->desks;}

/**
 * @brief Move user p_u from shopping are to a open cashdesk
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user who is moving
 */
void Market_FromShoppingToPay(Market * p_m, User * p_u) {
	Lock(&p_m->lock);
	//Remove user from shopping
	if( SQueue_remove(p_m->usersShopping, p_u, User_compare) != 1)
		err_quit("Impossible to find User %d in shopping area.", User_getId(p_u));
	User_setQueueStartTime(p_u, getCurrentTime());
	//Choose a random open caskdesk
	SQueue * desksOpen = NULL;
	void * aux = NULL;
	CashDesk * deskChoosen;
	if( (desksOpen = SQueue_init(-1)) == NULL) 
		err_quit("Malloc error.");
	for(int i=0;i < p_m->K; i++) {//Get id of all desks currently open
		if(CashDesk_getSate(&p_m->desks[i]) == DESK_OPEN){
			if(SQueue_push(desksOpen, &p_m->desks[i]) != 1)
				err_quit("An error occurred during open desk search. (1)");
		}
	}
	int r = getRandom(0,SQueue_dim(desksOpen)-1, &p_m->seed);
	if(SQueue_removePos(desksOpen, r, &aux) != 1)
		err_quit("An error occurred during open desk search. (2)");
	deskChoosen = (CashDesk *) aux;
	CashDesk_addUser(deskChoosen, p_u);
	Unlock(&p_m->lock);
}

/**
 * @brief Move user p_u from shopping are to a authorization queue
 * 
 * @param p_m reference to the market in which the action is performed
 * @param p_u user who is moving
 */
void Market_FromShoppingToAuth(Market * p_m, User * p_u) {
	Lock(&p_m->lock);
	//Remove user from shopping
	if( SQueue_remove(p_m->usersShopping, p_u, User_compare) != 1)
		err_quit("Impossible to find User %d in shopping area.", User_getId(p_u));
	User_setQueueStartTime(p_u, getCurrentTime());
	if(SQueue_push(p_m->usersAuthQueue, p_u) != 1)
		err_quit("Impossible to move User %d in authorization queue.", User_getId(p_u));
	Unlock(&p_m->lock);
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
	m->seed = time(NULL); //init seed for rand_r
	m->f_log = f_log;
	//Default values
	m->director = NULL;
	m->usersShopping = NULL;
	m->usersExit = NULL;
	m->usersAuthQueue = NULL;
	m->desks = NULL;
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
	res = pCheckContraint(m->K > 0, "{K>0}") != 1 ? 0:res;
	res = pCheckContraint(m->KS > 0 && m->KS <= m->K, "{0<KS<=K}") != 1 ? 0:res;
	res = pCheckContraint(m->C > 1, "{C>1}") != 1 ? 0:res;			
	res = pCheckContraint(m->E > 0 && m->E < m->C, "{0<E<C}") != 1 ? 0:res;
	res = pCheckContraint(m->T > 10, "{T>10}") != 1 ? 0:res;
	res = pCheckContraint(m->P > 0, "{P>0}") != 1 ? 0:res;
	res = pCheckContraint(m->S > 0, "{S>0}") != 1 ? 0:res;
	res = pCheckContraint(m->S1 > 0, "{S1>0}") != 1 ? 0:res;
	res = pCheckContraint(m->S2 > 0, "{S2>0}") != 1 ? 0:res;
	res = pCheckContraint(m->NP > 0, "{NP>0}") != 1 ? 0:res;
	
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
	if(	(m->usersShopping = SQueue_init(-1)) == NULL || 
		(m->usersExit = SQueue_init(-1)) == NULL ||
		(m->usersAuthQueue = SQueue_init(-1)) == NULL){
		err_msg("An error occurred during queues creation. Impossible to setup the market.");
		goto err;
	}

	//Init cash desk array
	if( (m->desks = malloc(m->K * sizeof(CashDesk))) == NULL){
		err_ret("An error occurred during memory allocation. (cashdesks array malloc)");
		goto err;
	}
	for(int i = 0;i < m->K; i++){
		if( CashDesk_init(m, &m->desks[i], i, (i<=m->K/2) ? DESK_OPEN:DESK_CLOSE) != 1 ) {
			err_msg("An error occurred during cashdesk creation. Impossible to setup the market.");
			goto err;
		}
	}

	//Init lock system
	if (pthread_mutex_init(&(m->lock), NULL) != 0) {
		err_msg("An error occurred during locking system initialization. Impossible to setup the market.");
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
		if(m->desks != NULL) {
			for(int i = 0;i < m->K; i++) CashDesk_delete(&m->desks[i]);
			free(m->desks);
		}
		if(isLockInit) pthread_mutex_destroy(&m->lock);
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
	SQueue_deleteQueue(p_m->usersAuthQueue, NULL);
	for(int i = 0;i < p_m->K; i++) CashDesk_delete(&p_m->desks[i]);
	free(p_m->desks);
	pthread_mutex_destroy(&p_m->lock);
    free(p_m);
    return 1;
}

void Market_Lock(Market * p_m) {Lock(&p_m->lock);}
void Market_Unlock(Market * p_m) {Unlock(&p_m->lock);}

void * Market_main(void * p_arg){
	Market * m = (Market *) p_arg;
	User * u_aux = NULL;
	int exit = 0; //count users exit until E is reached
	void * data;
	SQueue * newGroup = NULL;
	CashDesk * desks = Market_getDesks(m);
	Director * director = Market_getDirector(m);

	if((newGroup = SQueue_init(-1)) == NULL)
		err_quit("[Market]: An error occurred during market startup. (newGroup init failed)");
	
	//Start CashDesks Threads
	for(int i = 0; i < Market_getK(m);i ++){
		if(CashDesk_startThread(&desks[i]) != 0)
			err_quit("[Market]: An error occurred during desk thread start. (CashDesk startThread failed)");
	}

	//Start Director thread
	if(Director_startThread(Market_getDirector(m)) != 0)
		err_quit("[Market]: An error occurred during desk thread start. (CashDesk startThread failed)");

	//Create and add C users in shopping area
	for(int i = 0; i < Market_getC(m);i ++){
		if((u_aux = User_init(getRandom(0, Market_getP(m), Market_getSeed(m)), getRandom(10, Market_getT(m), Market_getSeed(m)), m)) == NULL)
			err_quit("[Market]: An error occurred during market startup. (User init failed)");
		if(User_startThread(u_aux) != 0)
			err_quit("[Market]: An error occurred during market startup. (User startThread failed)");
		SQueue_push(Market_getUsersShopping(m), u_aux);
	}

	//Wait E users exits
	while (1) {
		exit = 0;
	 	while (exit < Market_getE(m)){
			SQueue_popWait(Market_getUsersExit(m), &data);
			//Users thread in usersExit thread are not running
			u_aux = (User *) data;
			exit++;
			//Log user info and delete it.
			//It's important to free users memory during the simulation; otherwise there is
			//the risk to fill all the memory if the execution continue for a long time.
			User_log(u_aux, m->f_log);
			//Reset user structure for next reuse
			User_reset(u_aux, getRandom(0, Market_getP(m), Market_getSeed(m)), getRandom(10, Market_getT(m), Market_getSeed(m)), m);
			SQueue_push(newGroup, u_aux);
			if(exit == Market_getE(m)) {//E exits
				//Move all users in newGroup into shopping area
				while(SQueue_pop(newGroup, &data) != -2) {
					u_aux = (User *) data;
					if(User_startThread(u_aux) != 0)
						err_quit("[Market]: An error occurred during market startup. (User startThread failed)");
					SQueue_push(Market_getUsersShopping(m), u_aux);
				}
			}
		}
	}
		
    return (void *)NULL;
}



