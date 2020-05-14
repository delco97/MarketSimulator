
#include <configFileParser.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <utilities.h>

#define MAX_DIM_STR_CONF 1024

static const char g_comment[] = "//";

//Global variables configured according to the config file on startup.
long g_K; 	/**< Maximum number of open cashdesk. {K>0} */
long g_KS; 	/**< Number of open cashdesks at opening. {0<KS<=K} */
long g_C; 	/**< Maximum number of client allowed inside. {C >1} */
long g_E; 	/**< Number of users who need to exit before let other E users in. {0<E<C} */
long g_T; 	/**< Number of maximum ms spent by a single user in shopping area. {T>10} */
long g_P; 	/**< Maximum number of products that a single user can buy. {P>0} */
long g_S;	/**< Change queue evaluation interval (ms). {S>0}*/
long g_S1; 	/**< This threshold set the limit that tells to director if it's time to close a cashdesk.
				 In particular, S1 is maximum number of cashdesk with at most one user in queue.
				 When S1 is exceeded, it's time to close a cashdesk. {S1>0}*/
long g_S2; 	/**< This threshold set the limit that tells to director if it's time to open a cashdesk.
				 In particular, S2 is the maximum number of users in a single queue. So if there is at least
				 one queue with a number of user in queue equals or greater then S2. {S2>0} */
long g_NP; 	/**< Time required to process a single product. {NP>0} */

static int isDataValid = 0; /**< Tell if all configuration values are valid (isDataValid!=0).
								 isDataValid is set to 1 only after a successfull execution of parseConfigFile*/

/**
 * @brief Get position of string p_target insdie p_strings
 * 
 * @param p_strings array of strings
 * @param p_target string to search
 * @param p_dim number of strings inside p_strings
 * @return int: result code
 * [0;p_dim]: position of p_target inside p_strings (if found)
 * -1: failed
 */
static int getStringItemPos(const char * p_strings[], int p_dim, char * p_target){
	if(p_strings == NULL) return -1;
	for(int i = 0; i < p_dim; i++)
		if(strcmp(p_strings[i], p_target) == 0) return i;
	return -1;
}

/**
 * @brief 	Try to pares a configuration line by retrieving label and value of the configuration item.
 * 			
 * 			For a correct parsing p_str must have the following structure.:
 * 			    <config_label><p_limit><config_value>
 * 			If the p_str is configured as described above p_label will contain <config_label>
 * 			and p_value will contain <config_value>
 * 			
 * @param p_str string to analyze
 * @param p_limit limit string between configuration label and its value
 * @param p_value If the p_str is configured as described above p_value will contain <config_value>
 * @return int: result code
 * 1: ok
 * 0: failed
 */
static int parseConfigLine(char * p_str, char * p_limit, char * p_label, char * p_value) {
    char *tmpstr;
    char *token = strtok_r(p_str, p_limit, &tmpstr);
    int numtoken=0;
	
	while (token) {
		numtoken++;
		if(numtoken == 1){
			strcpy(p_label, token);
		}else{//numtoken == 2
			strcpy(p_value, token);
			break;
		}
		token = strtok_r(NULL, p_limit, &tmpstr);
    }
	return (numtoken == 2)?1:0;
}

/**
 * @brief   Try to parse as long value the string p_str_value. If p_str_value contains a valid long number, then
 *          value refereed by p_x is set to that value; otherwise an error will be shown.
 * 
 * @param p_x reference to memory location where the parsed value must be placed.
 * @param p_str_label <config_label>: config label of a config file line
 * @param p_str_value <config_value>: config value, in string format, of a config file line
 * @param p_line line number of the config file corresponding to the value to parse
 * @param p_status  reference to memory that will strore information about the parse result.
 *                  If an error occurred during parsing: *p_status = -1; otherwise *p_status = 1;
 * @return int: result code
 * 1: good parsing
 * 0: and error occurred during parsing
 */
static int parseLong(long * p_x, char * p_str_label, char * p_str_value, int p_line, int * p_status){
	int res = 1;
	long val = 0;	//store strtol result
	
	errno = 0;    // To distinguish success/failure after strtol call
	val = strtol(p_str_value, NULL, 10);
	//Check for various possible errors
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)){
		err_ret("Parsing error [line: %d]: %s require a positive intger number.\n", p_line, p_str_label);
		res = 0;
		*p_status = -1;
	}else{//Good format
		*p_x = val;
		*p_status = 1;
		res = 1;
	}
	return res;		
}
/**
 * @brief Utility function used for constraint checking (its purpose it's just for code lines reduction)
 * 
 * @param p_check result of check
 * @param p_contraint a string representation of the contraint being checked
 * @return int: return p_check
 */
static int checkContraint(int p_check, char * p_contraint){
	if(!p_check) fprintf(stderr,"Parsing error: constraint %s not satisfied.\n", p_contraint);
	return p_check;
}


static int isComment(char * p_line){
	char * aux = NULL;
	//A comment line must have g_comment string at the beginin
	if( (aux = strstr(p_line, g_comment)) != NULL && aux == p_line) return 1;
	return 0;
}

/**
 * @brief 	Try parse the configuration file p_configPath.
 * 			
 * After its execution all global variables "g_" related to config file will be initialized.
 * Duplicate configuration items are not allowed.
 * If a line start with g_comment
 * If one of the parameters is missing in the cofig file or if one of them doesn't respect requirements
 * the execution will t
 * 			
 * @param f path to a config file.
 * @return int: result code
 * 1: ok
 * 0: parsing failed
 */
int parseConfigFile(char * p_configPath){
	FILE * f_config = NULL;
    char line[MAX_DIM_STR_CONF]; //line of config file
	char str_label[MAX_DIM_STR_CONF];
	char str_value[MAX_DIM_STR_CONF];
	int pos = -1; //position of current configuration item inside confItem
	int line_count=0; //Current line number 
	int line_len;	//Current line length
	
	const char * confItem[] = { //configuration items labels
		"K","KS","C","E","T","P","S","S1","S2","NP"
	};	
	int numConfigItem = sizeof(confItem) / sizeof(confItem[0]);
	int * status; //Track status of each configuration item
					//(1: allready met in the config file; 0: not met yet;
					//-1: allready met in the config file with invalid value)
	int res=1; //function result

	//Check if the config file exist
	f_config = fopen(p_configPath, "r");
	if(f_config == NULL){
		err_sys("\nAn error occurred while opening the configuration file");
	}

	status = malloc(numConfigItem * sizeof(int));
	if(status == NULL) err_sys("\nAn error occurred during config file parsing.");
	
	memset(status, 0, numConfigItem * sizeof(int));
	while (fgets(line, MAX_DIM_STR_CONF, f_config) != NULL) {
		line_count++;
		line_len = strlen(line);
		if(line[line_len - 1] == '\n') line[line_len - 1] = '\0'; //remove new line
		if(isComment(line)) continue;
		//printf("\"%s\"\n", line);
		memset(str_value, 0, sizeof(str_label));
		memset(str_value, 0, sizeof(str_value));
		if(parseConfigLine(line, "=",str_label, str_value) == 0){
			fprintf(stderr,"Parsing error [line: %d]: invalid format.\n", line_count);
			res=0;
		}else{//String value retrieved
			pos = getStringItemPos(confItem, numConfigItem, str_label);
			if(pos == -1){//Ivalid config label
				fprintf(stderr,"Parsing error [line: %d]: unkown parameter %s.\n", line_count, str_label);
				res = 0;				
			}else{//Valid config label
				if(status[pos] != 0){//Parameter allready met in the config file
					fprintf(stderr,"Parsing error [line: %d]: %s was already defined in a previous line. No duplicates allowed.\n", line_count, str_label);
					res = 0;
				}else{//Parameter met for the first time inside the config file
					if (strcmp(str_label, "K") == 0) {
						res = parseLong(&g_K, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "KS") == 0){
						res = parseLong(&g_KS, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "C") == 0){
						res = parseLong(&g_C, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "E") == 0){
						res = parseLong(&g_E, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "T") == 0){
						res = parseLong(&g_T, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "P") == 0){
						res = parseLong(&g_P, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "S") == 0){
						res = parseLong(&g_S, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "S1") == 0){
						res = parseLong(&g_S1, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "S2") == 0){
						res = parseLong(&g_S2, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					} else if(strcmp(str_label, "NP") == 0){
						res = parseLong(&g_NP, str_label, str_value, line_count, &status[pos]) == 0 ? 0: res;
					}
				}
			}	
		}
	}
	//Check for missing configurations
	for(int i=0;i<numConfigItem;i++) {
		if(status[i] == 0){//configuration item not met in config file
			res=0;
			fprintf(stderr,"Parsing error: property %s is missing.\n", confItem[i]);
		}
	}
	//Only if all configuration item have been retrieved correctly
	//check if all constraints are satisfied
	if(res == 1){//Al parameters are set and have correct values.
		res = !checkContraint(g_K > 0, "{K>0}") ? 0:res; 						//{K>0}
		res = !checkContraint(g_KS > 0 && g_KS <= g_K, "{0<KS<=K}") ? 0:res; 	//{0<KS<=K}
		res = !checkContraint(g_C > 1, "{C>1}") ? 0:res;  						//{C>1}
		res = !checkContraint(g_E>0 && g_E < g_C, "{0<E<C}") ? 0:res;  			//{0<E<C}
		res = !checkContraint(g_T > 10, "{T>10}") ? 0:res;  					//{T>10}
		res = !checkContraint(g_P > 0, "{P>0}") ? 0:res;  						//{P>0}
		res = !checkContraint(g_S > 0, "{S>0}") ? 0:res;  						//{S>0}
		res = !checkContraint(g_S1 > 0, "{S1>0}") ? 0:res; 						//{S1>0}
		res = !checkContraint(g_S2 > 0, "{S2>0}") ? 0:res;  					//{S2>0}
		res = !checkContraint(g_NP > 0, "{NP>0}") ? 0:res;  					//{NP>0}
	}
    fclose(f_config);
	free(status);
	isDataValid = res;
	return res;
}

/**
 * @brief 	Print current configuration values.
 * 			This only works after a successful parseConfigFile execution.
 */
void printConfiguration(){
	if(!isDataValid) printf("Configuration items are not set.\n");
	else
		printf("K=%ld\nKS=%ld,\nC=%ld,\nE=%ld,\nT=%ld,\nP=%ld,\nS=%ld,\nS1=%ld,\nS2=%ld,\nNP=%ld",g_K,g_KS,g_C,g_E,g_T,g_P,g_S,g_S1,g_S2,g_NP);
}
