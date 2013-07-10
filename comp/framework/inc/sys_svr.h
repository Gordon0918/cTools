#ifndef _SYS_SVR_INC_
#define _SYS_SVR_INC_

#ifdef _cplusplus
extern "C" {
#endif

typedef struct _sys_cb_t {
    int (*init)  (void);
    int (*deinit)(void);
    int (*start) (void);
    int (*stop)  (void);
}sys_cb_t;

int sys_init(char           *prf_path,
             unsigned short  cmd_port,
             sys_cb_t       *sys_cb);

int sys_mainloop(void);

int sys_deinit(void);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_SVR_INC_ */

