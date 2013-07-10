/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gconfig.h
 *
 * Brief:
 *      geo data record module header file
 *
 * History:
 *      12/06/2012  create yufeng
 *****************************************************************************/

#ifndef _GCONFIG_INC_
#define _GCONFIG_INC_


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define VAL_INT      1
#define VAL_STRING  2

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/




/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/

extern void *cfg_open(char *cfg_file);
extern void *cfg_wopen(char *cfg_file);
extern int cfg_match(void *cfg_hnd, char *key, char keylen,int val_type, void *val_buf, int *val_size);
extern int cfg_close(void * cfg_hnd);

extern int test_read_config(void);

#endif
