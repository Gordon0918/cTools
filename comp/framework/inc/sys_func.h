#ifndef _SYS_FUNC_INC_
#define _SYS_FUNC_INC_

#ifdef _cplusplus
extern "C" {
#endif

#define FUNC_VALUE_MAX  16

typedef struct _sys_pair_t {
    char *tag;
    char *val;
}sys_pair_t;

typedef struct _sys_setting_t {
    char      *name;
    int        value_num;
    sys_pair_t values[FUNC_VALUE_MAX];
}sys_setting_t;

typedef struct _sys_func_op_t {
    int  (*info)(void *, int, char *);
    int  (*set )(void *, sys_setting_t *);
    int  (*clr_all)  (void *);
    int  (*clr_byidx)(void *, int);
    int  (*clr_bykey)(void *, char *);
    int  (*get_byidx)(void *, int,    sys_setting_t *);
    int  (*get_bykey)(void *, char *, sys_setting_t *);
}sys_func_op_t;

enum sys_func_type_e {
    SYS_FUNC_NULL = 0,
    SYS_FUNC_DICT = '%'
};

int   sys_func_init(void);
int   sys_func_deinit(void);
int   sys_func_register(const char *name, int capacity,
                        const sys_func_op_t *fop, void *udata);


void *sys_func_get_entry(const char *name);
void *sys_func_entry_root(void);
int   sys_func_entry_scan(void *entry, int (*cb)(void *, void *), void *uarg);
int   sys_func_entry_get_name(void *entry, int size, char *buf);

int   sys_entry_ver(void *entry);
int   sys_func_type(void *entry);
int   sys_func_capacity(void *entry);

int   sys_func_info(void *entry, int size, char *buf);
int   sys_func_set (void *entry, int size, char *setting);

int   sys_func_clr(void *entry);
int   sys_func_clr_bykey(void *entry, char *key);

int   sys_func_get_byidx(void *entry, int idx, int size, char *setting);
int   sys_func_get_bykey(void *entry, char *key, int size, char *setting);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_FUNC_INC_ */

