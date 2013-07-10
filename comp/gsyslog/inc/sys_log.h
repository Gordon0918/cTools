/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      sys_log.h
 *
 * History:
 *      20/09/2012  create ljx
 *****************************************************************************/
#ifndef _SYS_LOG_INC_
#define _SYS_LOG_INC_


#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/

#define SYSLOG_SWITCH_OFF 0
#define SYSLOG_SWITCH_ON  1

/******************************************************************************
* Struct & Union define                                                      *
*****************************************************************************/

typedef enum _sys_log_level_e
{
    LEVEL_SYS_EMERG = 0,
    LEVEL_SYS_ALERT ,
    LEVEL_SYS_CRIT ,
    LEVEL_SYS_ERR ,
    LEVEL_SYS_ALARM ,
    LEVEL_SYS_NOTICE ,
    LEVEL_INTERFERE ,
    LEVEL_SYS_DEBUG ,
    LEVEL_LAST
}sys_log_level_e;


/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/

/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/


extern int syslog_on();
extern int syslog_off();
extern int syslog_state();

extern void sys_log_init(char *dev_name);

extern int sys_log_switch_set(int hnd_id, int opt);

extern int sys_log(sys_log_level_e level, const char *ident, const char *format,...);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_LOG_INC_ */






