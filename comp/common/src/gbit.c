/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     gbit.c
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
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "stdint.h"

#include "gbit.h"
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

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/

void _gbit_print(void *blk, int len)
{
    int i = 0;
    unsigned char *p = (unsigned char *)blk;

    for(i = 0; i < len; i++)
    {
        printf("%02x",*p);
        p++;
    }
    printf("\r\n");
}



/* End of glog.c */
