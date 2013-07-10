#ifndef _SYS_USR_INC_
#define _SYS_USR_INC_

#ifdef _cplusplus
extern "C" {
#endif

#define SYS_USR_NUM     2

typedef struct _sys_usr_t {
    char name[16];
    char passwd[16];
    int  priority;
}sys_usr_t;

int        sys_user_init(void);
sys_usr_t *sys_user_def(void);
sys_usr_t *sys_user_find(char *usr_info);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_USR_INC_ */

