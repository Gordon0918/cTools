#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>

#include "sys_log.h"

#define OBSERVER 3

typedef void (*sys_log_func_t)(const unsigned int , void *, const char *, const char *, va_list );

typedef struct _syslog_observer_t
{
    int                 hnd_cnt;
    sys_log_func_t      hnd[OBSERVER];
    int                 hnd_onoff[OBSERVER];
    void                *hnd_args[OBSERVER];
}syslog_observer_t;

static syslog_observer_t g_observer[LEVEL_LAST];

int glevel[LEVEL_LAST] = {LOG_CONS,LOG_INFO,};



static void syslog_send2remote_server(const unsigned int level, void *args, const char *ident, const char *fmt, va_list ap)
{
    vsyslog(level, fmt, ap);
}

static void syslog_write2local_file(const unsigned int level, void *filename, const char *ident, const char *fmt, va_list ap)
{
    FILE *pf ;

    if(filename != NULL){
        pf = fopen(filename, "a+");
        if(pf == NULL) {
            return ;
        }
        vfprintf(pf, fmt, ap);
        fclose(pf);
    }else{
        pf = fopen("gsys_alarm_log", "a+");
        if(pf == NULL) {
            return ;
        }
        vfprintf(pf, fmt, ap);
        fclose(pf);
    }
}

static void syslog_print2tty(const unsigned int level, void *args,  const char *ident, const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
}

static int observer_register(int level, sys_log_func_t func, void *uarg)
{
    int index = 0;
    index = g_observer[level].hnd_cnt ;
    if(index < OBSERVER){
        g_observer[level].hnd[index] = func;
        g_observer[level].hnd_args[index] = uarg;
        g_observer[level].hnd_onoff[index] = SYSLOG_SWITCH_OFF;
        g_observer[level].hnd_cnt++;

        return 0;
    }

    return -1;
}

int sys_log_switch_set(int hnd_id, int opt)
{
    int level = 0;

    if(SYSLOG_SWITCH_OFF != opt && SYSLOG_SWITCH_ON != opt){
        return -1;
    }

    for(level = 0; level < LEVEL_LAST; level++){
        if(0 <= hnd_id && g_observer[level].hnd_cnt > hnd_id){
            g_observer[level].hnd_onoff[hnd_id] = opt;
        }
    }

    return 0;
}

int syslog_on()
{
   sys_log_switch_set(0, SYSLOG_SWITCH_ON);
   return 0;
}

int syslog_off()
{
   sys_log_switch_set(0, SYSLOG_SWITCH_OFF);
   return 0;
}

int syslog_state()
{
   return g_observer[0].hnd_onoff[0];
}

void sys_log_init(char *dev_name)
{
    int level, index;

    openlog(dev_name, LOG_PID|LOG_CONS|LOG_ODELAY, LOG_LOCAL0);

    for(level = 0; level < LEVEL_LAST; level++){
        g_observer[level].hnd_cnt = 0;
        for(index = 0; index < OBSERVER; index++){
            g_observer[level].hnd[index] = NULL;
            g_observer[level].hnd_args[index] = NULL;
            g_observer[level].hnd_onoff[index]= SYSLOG_SWITCH_OFF;
            g_observer[level].hnd_cnt = 0;

        }
    }

    for(level = 0; level < LEVEL_LAST; level++){
        observer_register(level, syslog_send2remote_server, NULL );
        observer_register(level, syslog_write2local_file, "geo.log");
        observer_register(level, syslog_print2tty, NULL );
    }
    return ;
}

void sys_log_deinit()
{
    closelog();
}

int sys_log(sys_log_level_e level,const char *ident, const char *format,...)
{
    if((level < 0)||(level >= LEVEL_LAST)){
        perror("level error");
        return -1;
    }

    if(0 == g_observer[level].hnd_cnt){
        return -1;
    }

    int index;
    va_list arg_ptr;
    for(index = 0; index < g_observer[level].hnd_cnt; index++){
        if(g_observer[level].hnd_onoff[index]){
            va_start(arg_ptr, format);
            g_observer[level].hnd[index](level, g_observer[level].hnd_args[index], ident, format, arg_ptr);
            va_end(arg_ptr);
        }
    }
    return 0;
}




