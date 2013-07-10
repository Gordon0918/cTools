/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_http.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _CONFIG_FILE_INC_
#define _CONFIG_FILE_INC_

#include <string.h>
#include <stdio.h>

#ifdef _cplusplus
extern "C" {
#endif


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/

#define FILE_ITEM_KEY_SIZE 20
#define FILE_ITEM_VALUE_SIZE 80
#define FILE_ITEM_MAXSIZE FILE_ITEM_VALUE_SIZE + FILE_ITEM_KEY_SIZE

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
FILE *config_file_open(char *filename);
int config_file_read(FILE *config_file, char *key, char *value);
int config_file_append(FILE *config_file,char *key,char *value);
int config_file_close(FILE *config_file);
int http_filter_config_load(char * filename,char *seg_key,int (* handle)(char *));



/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/

#ifdef _cplusplus
}
#endif

#endif /* _GPARSER_HTTP_INC_ */

/* End of gparser_http.h */



