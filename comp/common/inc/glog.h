/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gipc.h
 *
 * Brief:
 *      geo ipc module header file
 *
 * History:
 *      05/06/2012  create xing
 *****************************************************************************/

#ifndef _GLOG_INC_
#define _GLOG_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GLOG_FATAL_ERROR          0
#define GLOG_ERROR                1
#define GLOG_SYSINFO              1
#define GLOG_WARNING              2
#define GLOG_MESSAGE              3
#define GLOG_DEBUG                4

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/
extern int glog_level;

extern int acct_debug;
extern int ndas_debug;
extern int ndps_debug;
extern int parse_http_debug;
extern int cmd_debug;

extern int glog_print(int level, int module, char *str, ...);

/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/

#define GLOG_PRINT(level, module, fmt, args...) \
    do{\
        if((level) <= glog_level && (module)) \
            glog_print(level, module, fmt,##args);\
      }while(0)    
    

 


#ifdef _cplusplus
}
#endif

#endif /* _GIPC_INC_ */

/* End of glog.h */

