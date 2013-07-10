#include <stdio.h>
#include <string.h>

#include <sys_cmd_intf.h>
#include <sys_func.h>
#include <sys_cfg.h>

int sys_cmd_save(int ac, char **av, int dat_len, char *dat_buf)
{
    int result;

    if(ac > 2){
        result = sys_cfg_save(av[1], av[2]);

        return sys_cmd_return(result, 0, NULL);
    }else{
        //remote save 
        result = -1;

        return sys_cmd_return(result, 0, NULL);
    }
}

int sys_cmd_load(int ac, char **av, int dat_len, char *dat_buf)
{
    int   result;
    char *retstr = "OK";

    if(ac > 1){
        int i;
        int ret = 0;

        for(i = 1; i < ac; i++){
            ret = sys_cfg_load(av[i]);
            if(ret != 0){
                retstr = av[i];
                break;
            }
        }
        
        result = ret;
    }else if(dat_len > 0 && dat_buf){
        result = sys_cfg_load_mem(dat_len, dat_buf);
    }else{
        result = -1;
    }

    return sys_cmd_return(result, strlen(retstr), retstr);
}

