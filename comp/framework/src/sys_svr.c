/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     .c
 * 
 * Brief:
 * 
 * 
 * History:
 *      05/06/2012 create dinglixing
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 * Declaration                                                               *
 *****************************************************************************/
#include <sys_svr.h>
#include <sys_func.h>
#include <sys_cmd_intf.h>
#include <sys_usr.h>


static sys_cb_t sys_cb = {NULL, };
/******************************************************************************
 * Local function define                                                     *
 *****************************************************************************/
static int sys_start(void)
{
    int sys_cmd_install(void);
    
    sys_cmd_install();
    
    if(sys_cb.start && 0 != sys_cb.start()){
        return -1;
    }else{
        return 0;
    }
}

static int sys_stop(void)
{
    if(sys_cb.stop && 0 != sys_cb.stop()){
        return -1;
    }else{
        return 0;
    }
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
int sys_init(char *profile_path, unsigned short cmd_port, sys_cb_t *usr_cb)
{
    if(usr_cb == NULL){
        return -1;
    }

    sys_func_init();
    sys_user_init();
    sys_cmd_init(NULL, cmd_port);
    
    sys_cb = *usr_cb;

    if(sys_cb.init && 0 != sys_cb.init()){
        return -1;
    }
    
    return 0;
}

int sys_deinit(void)
{
    if(sys_cb.deinit && 0 != sys_cb.deinit()){
        return -1;
    }

    sys_cmd_deinit();
    sys_func_deinit();
    
    return 0;
}

int sys_mainloop(void)
{
    sys_start();

    sys_cmd_mainloop();
    
    sys_stop();
    
    return 0;
}

