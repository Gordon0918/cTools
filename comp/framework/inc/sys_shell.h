#ifndef _SYS_SHELL_INC_
#define _SYS_SHELL_INC_

#ifdef _cplusplus
extern "C" {
#endif

typedef int (*sys_sh_proc_t)(int ac, char **av);


int sys_cli_sh_init(void);
int sys_cli_sh_deinit(void);
int sys_cli_sh_register(const char *cmd_name, sys_sh_proc_t cmd_proc);
int sys_cli_sh_mainloop(void);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_SHELL_INC_ */

