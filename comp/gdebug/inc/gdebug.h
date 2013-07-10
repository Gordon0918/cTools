/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gdegug.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _GDEBUG_INC_
#define _GDEBUG_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/



/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/
int gpthread_mutex_lock(pthread_mutex_t *lock, const char *file, const char *func, int line);
int gpthread_mutex_unlock(pthread_mutex_t *lock);


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
#ifdef G_DEBUG
#define pthread_mutex_lock(lock)    gpthread_mutex_lock(lock, __FILE__, __func__, __LINE__) 
#define pthread_mutex_unlock(lock)  gpthread_mutex_unlock(lock) 
#endif

#ifdef _cplusplus
}
#endif

#endif /* _GUDEBUG_INC_ */
