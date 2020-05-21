/**
 * @file Config.h
 * @brief Header file for Config.c
 */
#ifndef	_CONFIG_H
#define	_CONFIG_H

#include <stdio.h>

/**
 * @brief Max string length expected from a config file
 */
#define MAX_DIM_STR_CONF 1024

int Config_getValue(FILE * p_f, const char * p_key, char * p_buff);
int Config_checkFile(FILE * p_f);
int Config_parseLong(long * p_x, char * p_str_value);

#endif	/* _CONFIG_H */
