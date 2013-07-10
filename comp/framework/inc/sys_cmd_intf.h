#ifndef _SYS_IF_INC_
#define _SYS_IF_INC_

#ifdef _cplusplus
extern "C" {
#endif

typedef int (*sys_cmd_proc_t)(int ac, char **av, int dlen, char *dbuf);

int sys_cmd_init(const char *cmd_addr, unsigned short  cmd_port);
int sys_cmd_deinit(void);
int sys_cmd_register(const char *cmd_name, sys_cmd_proc_t cmd_proc);
int sys_cmd_install(void);

int sys_cmd_mainloop(void);
int sys_cmd_return(int result, int rlen, char *rdat);
int sys_cmd_return_dync(int result, int (*dat_gen)(void*, int, char *), void *uarg);

int sys_cmd_rcall_init(const char *ip, unsigned short port);
int sys_cmd_rcall(int ac, char **av, int dlen, char *dbuf, int *rlen, char **rbuf);
int sys_cmd_rlogin(const char *user, const char *passwd);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_IF_INC_ */

