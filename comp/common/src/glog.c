/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     glog.c
 * 
 * Brief:
 * 
 * 
 * History:
 * 
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include <glog.h>

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/


/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/
int  glog_level;

int acct_debug = 1;
int ndas_debug = 1;
int ndps_debug = 1;
int parse_http_debug = 1;
int cmd_debug = 1;

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/

int glog_print(int level, int module, char *str, ...)
{
    va_list args;
    
    va_start(args, str);

    vfprintf(stdout, str, args);

    va_end(args);

    return 0;
}



/* End of glog.c */

