#include <stdio.h>
#include <string.h>

#include <sys_cmd_intf.h>
#include <sys_func.h>

int sys_cmd_set(int ac, char **av, int dat_len, char *dat)
{
    void *entry;
    int   result;

    if(ac < 3){
        return sys_cmd_return(-1, 0, NULL);
    }
    
    entry = sys_func_get_entry(av[1]);
    if(entry == NULL || SYS_FUNC_NULL == sys_func_type(entry)){
        return sys_cmd_return(-1, strlen("Invalid target"), "Invalid target");
    }

    result = sys_func_set(entry, strlen(av[2]), av[2]);

    return sys_cmd_return(result, 0, NULL);
}

