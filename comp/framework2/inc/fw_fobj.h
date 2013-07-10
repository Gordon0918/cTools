/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      fw_fobj.h
 *
 * Brief:
 *      framework function object header file
 *
 * History:
 *      25/10/2012  create dinglixing
 *****************************************************************************/

#ifndef _FW_FOBJ_INC_
#define _FW_FOBJ_INC_

#include <list.h>

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types & Defines                                                            *
 *****************************************************************************/
typedef struct _fw_fobj_attr_t {
    const char *name;
    int       (*set)(void *obj_dat, const char *vstr);
    int       (*get)(void *obj_dat, char *vbuf, int size);
}fw_fobj_attr_t;

typedef struct _fw_fobj_proc_t {
    const char *name;
    int       (*proc)(void *obj_dat, int argc, char **argv, char *retbuf, int size);
}fw_fobj_proc_t;

typedef struct _fw_fobj_desc_t {
    int         attr_nr;
    int         proc_nr;
    const fw_fobj_attr_t *attrs;
    const fw_fobj_proc_t *procs;
    void     *(*init) (int argc, char **argv);
    int       (*deinit)(void *);
}fw_fobj_desc_t;

typedef struct _fw_fobj_finfo_t {
    const char          *name;
    int                  inst_max;
    int                  inst_num;
    const fw_fobj_desc_t *func_desc;
    const fw_fobj_desc_t *inst_desc;
}fw_fobj_finfo_t;

typedef struct _fw_fobj_iinfo_t {
    const char          *base;
    const char          *name;
    const fw_fobj_desc_t *desc;
    void                *inst;
}fw_fobj_iinfo_t;

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/


/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int fw_fobj_init(void);
int fw_fobj_deinit(void);
int fw_fobj_register(const char *name,
                     const fw_fobj_desc_t *func_desc,
                     const fw_fobj_desc_t *inst_desc,
                     int inst_max,
                     int ac, char **av);

int fw_fobj_cnt(void);
int fw_fobj_id(const char *name);
int fw_fobj_info4id(int func_id, fw_fobj_finfo_t *info);
int fw_fobj_info4name(const char *name, fw_fobj_finfo_t *info);

int fw_fobj_set(int func_id, const char *attr, const char *val);
int fw_fobj_get(int func_id, const char *attr, char *buf, int size);
int fw_fobj_proc(int func_id, const char *proc, int ac, char **av, char *buf, int size);

int fw_fobj_inst_info(void *inst, fw_fobj_iinfo_t *info);
int fw_fobj_inst_info4id(int func_id, int inst_id, fw_fobj_iinfo_t *info);
int fw_fobj_inst_info4name(const char *func_name, const char *inst_name, fw_fobj_iinfo_t *info);

void *fw_fobj_inst_create(const char *func_name, const char *inst_name, int ac, char **av);
int   fw_fobj_inst_destory(void *inst);
void *fw_fobj_inst(const char *func_name, const char *inst_name);

int   fw_fobj_inst_set(void *inst, const char *attr, const char *val);
int   fw_fobj_inst_get(void *inst, const char *attr, char *buf, int size);
int   fw_fobj_inst_proc(void *inst, const char *proc, int ac, char **av, char *buf, int size);

#ifdef _cplusplus
}
#endif

#endif /* _FW_FOBJ_INC_ */

/* End of fw_fobj.h */

