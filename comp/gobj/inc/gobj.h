/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gobj.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _GOBJ_INC_
#define _GOBJ_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GOBJ_INVALID ((void *)0)


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef void * gobj_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int    gobj_init  (int max);
int    gobj_deinit(void);
gobj_t gobj_create(void *value, void (*destruct)(void *));
gobj_t gobj_dup   (gobj_t gobj);
void * gobj_value (gobj_t gobj);
void   gobj_delete(gobj_t robj);

#ifdef _cplusplus
}
#endif

#endif /* _GOBJ_INC_ */

