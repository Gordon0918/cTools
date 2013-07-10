#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include <sys_shell.h>

#define CMD_BUF_SIZE    1024
#define CMD_ARG_MAX     16
#define CMD_ARG_SIZE    32
#define CMD_MAX         128

typedef struct _cmd_entry_t {
    const char       *cmd_name;
    sys_sh_proc_t     cmd_proc;
}cmd_sh_entry_t;

static char             cmd_buf[CMD_BUF_SIZE];
static char             cmd_arg_buf[CMD_ARG_MAX][CMD_ARG_SIZE];
static char            *cmd_arg[CMD_ARG_MAX];
static int              cmd_arg_num;

static int              cmd_num = 0;
static cmd_sh_entry_t   cmd_entrys[CMD_MAX];
static int              cmd_done = 0;

static int cmd_default(int ac, char **av)
{
    printf("Unknown cmd: %s\n", av[0]);
    return -1;
}

static int cmd_exit(int ac, char **av)
{
    cmd_done = 1;
    return 0;
}

static sys_sh_proc_t cmd_find(char *cmd_name)
{
    int i = 0;

    if(strcmp(cmd_name, "exit") == 0){
        return cmd_exit;
    }
    
    for (i = 0; i < cmd_num; i++) {
        if (strcmp(cmd_name,cmd_entrys[i].cmd_name) == 0) {
            return cmd_entrys[i].cmd_proc;
        }
    }
    
    return cmd_default;
}

static int cmd_wait(void)
{
    char *line;
    char *token;
    int i;

    line = fgets(cmd_buf, CMD_BUF_SIZE - 1, stdin);
    if(line == NULL){
        return -1;
    }

    token = strtok(line, " \t\r\n");
    for(i = 0; i < CMD_ARG_MAX && token; i++, token = strtok(NULL, " \t\r\n")){
        strncpy(cmd_arg[i], token, CMD_ARG_SIZE);
    }

    if(i){
        cmd_arg_num = i;
        return 0;
    }else{
        return -1;
    }
}

static int cmd_proc(void)
{
    sys_sh_proc_t cmd_func;

    cmd_func = cmd_find(cmd_arg[0]);
    
    return cmd_func(cmd_arg_num, cmd_arg);
}

int sys_cli_sh_init(void)
{
    int i;

    for(i = 0; i < CMD_ARG_MAX; i++){
        cmd_arg[i] = cmd_arg_buf[i];
    }
    
    return 0;
}

int sys_cli_sh_deinit(void)
{
    return 0;
}

int sys_cli_sh_register(const char *cmd_name, sys_sh_proc_t cmd_proc)
{
    cmd_sh_entry_t *pcmd;
    
    if(cmd_num >= CMD_MAX){
        return -1;
    }

    pcmd = &cmd_entrys[cmd_num++];

    pcmd->cmd_name = cmd_name;
    pcmd->cmd_proc = cmd_proc;
    
    return 0;
}

int sys_cli_sh_mainloop(void)
{
    while(!cmd_done){
        int cmd_ret;
        
        printf("->");
        if(0 != cmd_wait()){
            printf("\n");
            continue;
        }

        cmd_ret = cmd_proc();
        if(0 != cmd_ret){
            printf("FAIL: %d\n", cmd_ret);
        }
    }
    return 0;
}

