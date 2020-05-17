
#include <Config.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <utilities.h>
#include <SQueue.h>

static const char g_comment[] = "//";
static const char g_separatorKeyValue[] = "=";

static int isComment(char * p_line){
	char * aux = NULL;
	//A comment line must have g_comment string at the beginin
	if( (aux = strstr(p_line, g_comment)) != NULL && aux == p_line) return 1;
	return 0;
}

static int cmpStr(void * p_1, void * p_2){
	return strcmp((char *) p_1, (char *) p_2);
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
static int parseConfigLine(char * p_str, const char * p_limit, char * p_label, char * p_value) {
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
 * @brief 	Try to find value associated to the key p_key in the config file p_f.
 * 
 * If a line start with g_comment it is ignored.
 * 			
 * @param p_f target file.
 * @param p_key key to find
 * @param p_buff where the value is placed if p_key is found (must have a dim of MAX_DIM_STR_CONF)
 * @return int: result code:
 * 0: p_key not found
 * 1: p_key found
 */
int Config_getValue(FILE * p_f, const char * p_key, char * p_buff){
    char line[MAX_DIM_STR_CONF]; //line of config file
	char str_label[MAX_DIM_STR_CONF];
	char str_value[MAX_DIM_STR_CONF];
	int line_len;	//Current line length
	
	rewind(p_f); //rewind the file at begining
	
	while (fgets(line, MAX_DIM_STR_CONF, p_f) != NULL) {
		line_len = strlen(line);
		if(line[line_len - 1] == '\n') line[line_len - 1] = '\0'; //remove new line (if present)
		if(isComment(line)) continue;
		memset(str_value, 0, sizeof(str_label));
		memset(str_value, 0, sizeof(str_value));
		if(	parseConfigLine(line, g_separatorKeyValue, str_label, str_value) == 1 && 
			strcmp(p_key, str_label) == 0){ //Good line format and p_key found
			strcpy(p_buff, str_value);
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Check if p_f is a valid configuration file.
 * 
 * @param p_f 
 * @return int result code:
 * 1: is a a valid configuration file
 * 0: is not a a valid configuration file
 */
int Config_checkFile(FILE * p_f){
    char line[MAX_DIM_STR_CONF]; //line of config file
	char str_label[MAX_DIM_STR_CONF];
	char str_value[MAX_DIM_STR_CONF];
	int line_count=0; //Current line number 
	int line_len;	//Current line length
	int res=1; //function result
	char * aux;
	SQueue  * q_labels = SQueue_init(-1); //track of valid lables encountered

	rewind(p_f); //rewind the file at begining
	while (fgets(line, MAX_DIM_STR_CONF, p_f) != NULL) {
		line_count++;
		line_len = strlen(line);
		if(line[line_len - 1] == '\n') line[line_len - 1] = '\0'; //remove new line
		if(isComment(line)) continue;
		memset(str_value, 0, sizeof(str_label));
		memset(str_value, 0, sizeof(str_value));
		if(parseConfigLine(line, g_separatorKeyValue, str_label, str_value) != 1){
			fprintf(stderr,"Parsing error [line: %d]: invalid format.\n", line_count);
			res=0;
		}else{//Good line format
			if(SQueue_find(q_labels, str_label, cmpStr) < 0){//Label never encoutereed
				if((aux = malloc(MAX_DIM_STR_CONF * sizeof(char))) == NULL) 
					err_sys("An error occurred during a malloc");
				strcpy(aux, str_label);
				SQueue_push(q_labels, aux);
			} else{ //Label already encountered
				fprintf(stderr,"Parsing error [line: %d]: label %s is already defined in a previous line.\n", line_count, str_label);
				res = 0;
			}
		}
	}
	SQueue_deleteQueue(q_labels, free);
	return res;
}

/**
 * @brief   Try to parse as long value the string p_str_value. Only if p_str_value contains a valid long number, the
 *          value refereed by p_x is set to that value;.
 * 
 * @param p_x reference to memory location where the parsed value must be placed.
 * @param p_str_value string value to parse.
 * @return int: result code
 * 1: good parsing (p_x is set)
 * 0: and error occurred during parsing (p_x is not set). Note that "errno" is set.
 */
int Config_parseLong(long * p_x, char * p_str_value){
	int res = 1;
	long val = 0;	//store strtol result
	
	errno = 0;    // To distinguish success/failure after strtol call
	val = strtol(p_str_value, NULL, 10);
	//Check for various possible errors
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)){
		err_ret("Parsing error: %s can't be parsed as long.\n");
		res = 0;
	}else{//Good parsing
		*p_x = val;
		res = 1;
	}
	return res;		
}
