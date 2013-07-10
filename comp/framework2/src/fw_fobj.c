/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     fw_fobj.c
 *
 * Brief:
 *
 *
 * History:
 *      25/10/2012 create dinglixing
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <fw_fobj.h>

/******************************************************************************
 * Types & Defines                                                              *
 *****************************************************************************/
#define FOBJ_MAX        64
#define INST_MAX        256
#define OBJ_INST_MAGIC  0x20121025

typedef struct _fw_fobj_t {
    const char           *name;
    int                   func_type;
    void                 *func_data;
    const fw_fobj_desc_t *func_desc;
    const fw_fobj_desc_t *inst_desc;
    int                   inst_max;
    int                   inst_cnt;
    struct list_head      inst_head;
}fw_fobj_t;

typedef struct _fw_fobj_inst_t {
    int                 magic;
    int                 type;
    void               *data;
    fw_fobj_t          *fobj;
    struct list_head    list;
    char                name[16];
}fw_fobj_inst_t;

/******************************************************************************
 * Local veriables                                                            *
 *****************************************************************************/
static int              fobj_used = 0;
static fw_fobj_t        fobjs[FOBJ_MAX];

static int              inst_used = 0;
static fw_fobj_inst_t   insts[INST_MAX];
static struct list_head insts_free;

/******************************************************************************
 * Local functions                                                            *
 *****************************************************************************/
static fw_fobj_t *fobj_seek(const char *name)
{
    int i;

    for(i = 0; i < fobj_used; i++){
        fw_fobj_t *fobj = &fobjs[i];

        if(strcmp(name, fobj->name) == 0){
            return fobj;
        }
    }

    return NULL;
}

static inline fw_fobj_t *fobj_seek4id(int func_id)
{
    if(func_id < fobj_used){
        return &fobjs[func_id];
    }

    return NULL;
}

static const fw_fobj_attr_t *fobj_attr(fw_fobj_t *fobj, const char *attr)
{
    const fw_fobj_desc_t *desc;
    int i;

    desc = fobj->func_desc;
    if(desc == NULL){
        return NULL;
    }

    for(i = 0; i < desc->attr_nr; i++){
        if(strcmp(attr, desc->attrs[i].name) == 0){        
            return &desc->attrs[i];
        }
    }

    return NULL;
}

static const fw_fobj_proc_t *fobj_proc(fw_fobj_t *fobj, const char *proc)
{
    const fw_fobj_desc_t *desc;
    int i;

    desc = fobj->func_desc;
    if(desc == NULL){
        return NULL;
    }

    for(i = 0; i < desc->proc_nr; i++){
        if(strcmp(proc, desc->procs[i].name) == 0){
            return &desc->procs[i];
        }
    }

    return NULL;
}

static inline fw_fobj_inst_t *inst_alloc(void)
{
    fw_fobj_inst_t *inst = NULL;

    if(!list_empty(&insts_free)){
        struct list_head *node;

        node = insts_free.next;
        list_del_init(node);

        inst = list_entry(node, fw_fobj_inst_t, list);
    }

    return inst;
}

static inline int inst_free(fw_fobj_inst_t *inst)
{
    list_add(&inst->list, &insts_free);
    return 0;
}

static inline int inst_check(fw_fobj_inst_t *inst)
{
    if((inst - insts) >= INST_MAX){
        return -1;
    }

    if(inst->magic != OBJ_INST_MAGIC){
        return -1;
    }

    return 0;
}

static const fw_fobj_attr_t *inst_attr(fw_fobj_inst_t *inst, const char *attr)
{
    const fw_fobj_desc_t *desc;
    int i;

    desc = inst->fobj->inst_desc;
    for(i = 0; i < desc->attr_nr; i++){
        if(strcmp(attr, desc->attrs[i].name) == 0){
            return &desc->attrs[i];
        }
    }

    return NULL;
}

static const fw_fobj_proc_t *inst_proc(fw_fobj_inst_t *inst, const char *proc)
{
    const fw_fobj_desc_t *desc;
    int i;

    desc = inst->fobj->inst_desc;
    for(i = 0; i < desc->proc_nr; i++){
        if(strcmp(proc, desc->procs[i].name) == 0){
            return &desc->procs[i];
        }
    }

    return NULL;
}

/******************************************************************************
 * Global functions                                                         *
 *****************************************************************************/
int fw_fobj_init(void)
{
    int i;

    fobj_used = 0;
    for(i = 0; i < FOBJ_MAX; i++){
        fw_fobj_t *fobj = &fobjs[i];

        memset(fobj, 0, sizeof(fw_fobj_t));
        INIT_LIST_HEAD(&fobj->inst_head);
    }

    inst_used = 0;
    INIT_LIST_HEAD(&insts_free);
    for(i = 0; i < INST_MAX; i++){
        fw_fobj_inst_t *inst = &insts[i];

        memset(inst, 0, sizeof(fw_fobj_inst_t));
        inst->magic = OBJ_INST_MAGIC;
        inst->data = NULL;
        inst->type = 0;
        list_add(&inst->list, &insts_free);
    }

    return 0;
}

int fw_fobj_deinit(void)
{
    int i;

    for(i = 0; i < fobj_used; i++){
        fw_fobj_t *fobj = &fobjs[i];
        struct list_head *pos, *next;

        list_for_each_safe(pos, next, &fobj->inst_head){
            fw_fobj_inst_t *inst = list_entry(pos, fw_fobj_inst_t, list);

            fw_fobj_inst_destory(inst);
        }

        if(fobj->func_desc && fobj->func_desc->deinit){
            fobj->func_desc->deinit(fobj->func_data);
        }
    }

    return 0;
}

int fw_fobj_register(const char *name,
                     const fw_fobj_desc_t *func_desc,
                     const fw_fobj_desc_t *inst_desc,
                     int inst_max,
                     int ac, char **av)
{
    if(fobj_used < FOBJ_MAX){
        int   func_id;
        void *func_data = NULL;

        if(func_desc && func_desc->init){
            func_data = func_desc->init(ac, av);
        }
        func_id = fobj_used;
        fw_fobj_t *fobj = &fobjs[fobj_used++];
        fobj->name = name;
        fobj->func_type = 0; //not used
        fobj->func_data = func_data;
        fobj->func_desc = func_desc;
        fobj->inst_desc = inst_desc;
        fobj->inst_cnt = 0;
        fobj->inst_max = inst_max;
        return func_id;
    }

    return -1;
}

int fw_fobj_cnt(void)
{
    return fobj_used;
}

int fw_fobj_id(const char *name)
{
    int func_id;

    for(func_id = 0; func_id < fobj_used; func_id++){
        fw_fobj_t *fobj = &fobjs[func_id];

        if(strcmp(fobj->name, name) == 0){
            return func_id;
        }
    }

    return -1;
}

int fw_fobj_info4id(int func_id, fw_fobj_finfo_t *info)
{
    if(info && func_id < fobj_used){
        fw_fobj_t *fobj = &fobjs[func_id];

        info->name      = fobj->name;
        info->inst_max  = fobj->inst_max;
        info->inst_num  = fobj->inst_cnt;
        info->func_desc = fobj->func_desc;
        info->inst_desc = fobj->inst_desc;

        return 0;
    }else{
        return -1;
    }
}

int fw_fobj_info4name(const char *name, fw_fobj_finfo_t *info)
{
    int func_id;

    for(func_id = 0; func_id < fobj_used; func_id++){
        fw_fobj_t *fobj = &fobjs[func_id];

        if(strcmp(fobj->name, name) == 0){
            info->name      = fobj->name;
            info->inst_max  = fobj->inst_max;
            info->inst_num  = fobj->inst_cnt;
            info->func_desc = fobj->func_desc;
            info->inst_desc = fobj->inst_desc;
            return 0;
        }
    }

    return -1;
}

int fw_fobj_get(int func_id, const char * attr, char * buf, int size)
{
    fw_fobj_t *fobj;
    const fw_fobj_attr_t *desc;

    fobj = fobj_seek4id(func_id);
    if(fobj == NULL){
        return -1;
    }

    desc = fobj_attr(fobj, attr);
    if(desc == NULL){
        return -1;
    }

    return desc->get ? desc->get(fobj->func_data, buf, size) : -1;
}

int fw_fobj_set(int func_id, const char * attr, const char * val)
{
    fw_fobj_t *fobj;
    const fw_fobj_attr_t *desc;

    fobj = fobj_seek4id(func_id);
    if(fobj == NULL){
        return -1;
    }

    desc = fobj_attr(fobj, attr);
    if(desc == NULL){
        return -1;
    }

    return desc->set ? desc->set(fobj->func_data, val) : -1;
}

int fw_fobj_proc(int func_id, const char * proc, int ac, char * * av, char * buf, int size)
{
    fw_fobj_t *fobj;
    const fw_fobj_proc_t *desc;

    fobj = fobj_seek4id(func_id);
    if(fobj == NULL){
        return -1;
    }

    desc = fobj_proc(fobj, proc);
    if(desc == NULL){
        return -1;
    }

    return desc->proc ? desc->proc(fobj->func_data, ac, av, buf, size) : -1;
}

void *fw_fobj_inst(const char *func_name, const char *inst_name)
{
    int        func_id;
    fw_fobj_t *fobj = NULL;
    struct list_head *pos;

    for(func_id = 0; func_id < fobj_used; func_id++){
        fw_fobj_t *cur = &fobjs[func_id];

        if(strcmp(cur->name, func_name) == 0){
            fobj = cur;
            break;
        }
    }

    if(fobj == NULL){
        return NULL;
    }

    __list_for_each(pos, &fobj->inst_head){
        fw_fobj_inst_t *inst = list_entry(pos, fw_fobj_inst_t, list);

        if(strcmp(inst->name, inst_name) == 0){
            return inst;
        }
    }
    return NULL;
}

void *fw_fobj_inst_create(const char *func_name, const char *inst_name, int ac, char **av)
{
    fw_fobj_t       *fobj;
    fw_fobj_inst_t   *inst;
    void            *inst_dat;

    fobj = fobj_seek(func_name);
    if(fobj == NULL){
        return NULL;
    }

    if(fobj->inst_cnt >= fobj->inst_max){
        return NULL;
    }

    inst = inst_alloc();
    if(inst == NULL){
        return NULL;
    }

    inst_dat = fobj->inst_desc->init(ac, av);
    if(inst_dat == NULL){
        inst_free(inst);
        return NULL;
    }

    inst->type = 0; //noused
    inst->data = inst_dat;
    inst->fobj = fobj;
    strncpy(inst->name, inst_name, 16);
    list_add_tail(&inst->list, &fobj->inst_head);

    fobj->inst_cnt++;

    return inst;
}

int fw_fobj_inst_destory(void *obj)
{
    fw_fobj_inst_t   *inst = (fw_fobj_inst_t *)obj;

    if(0 != inst_check(inst)){
        return -1;
    }

    assert(inst->fobj->inst_cnt > 0);
    if(0 != inst->fobj->inst_desc->deinit(inst->data)){
        return -1;
    }

    //unlink from fclass
    list_del(&inst->list);
    inst->fobj->inst_cnt--;

    inst_free(inst);

    return 0;
}

int fw_fobj_inst_info(void *obj, fw_fobj_iinfo_t *info)
{
    fw_fobj_inst_t   *inst = (fw_fobj_inst_t *)obj;

    if(0 != inst_check(inst)){
        return -1;
    }
    info->base = inst->fobj->name;
    info->name = inst->name;
    info->desc = inst->fobj->inst_desc;
    info->inst = inst;

    return 0;
}

int fw_fobj_inst_info4name(const char *func_name, const char *inst_name, fw_fobj_iinfo_t *info)
{
    if(info && func_name && inst_name) {
        int func_id;
        func_id = fw_fobj_id(func_name);
        if(func_id < 0) {
            return -1;
        }
        fw_fobj_t *fclass = &fobjs[func_id];
        struct list_head *pos;
        __list_for_each(pos, &fclass->inst_head){
                fw_fobj_inst_t *inst;
                inst = list_entry(pos, fw_fobj_inst_t, list);
                if(strcmp(inst_name,inst->name) == 0) {
                    info->base = inst->fobj->name;
                    info->name = inst->name;
                    info->desc = inst->fobj->inst_desc;
                    info->inst = inst;
                    return 0;
                }
        }
    }

    return -1;
}


int fw_fobj_inst_info4id(int class_id, int inst_id, fw_fobj_iinfo_t *info)
{
    if(info && class_id < fobj_used){
        fw_fobj_t *fclass = &fobjs[class_id];
        struct list_head *pos;
        int id = 0;

        __list_for_each(pos, &fclass->inst_head){
            if(id++ == inst_id){
                fw_fobj_inst_t *inst;

                inst = list_entry(pos, fw_fobj_inst_t, list);

                info->base = inst->fobj->name;
                info->name = inst->name;
                info->desc = inst->fobj->inst_desc;
                info->inst = inst;
                return 0;
            }
        }
    }
    return -1;
}



int fw_fobj_inst_set(void *obj, const char *attr, const char *val)
{
    fw_fobj_inst_t *inst = (fw_fobj_inst_t *)obj;
    const fw_fobj_attr_t *desc;

    if(0 != inst_check(inst)){
        return -1;
    }

    desc = inst_attr(inst, attr);
    if(desc == NULL){
        return -1;
    }

    return desc->set ? desc->set(inst->data, val) : -1;
}

int fw_fobj_inst_get(void *obj, const char *attr, char *buf, int size)
{
    fw_fobj_inst_t *inst = (fw_fobj_inst_t *)obj;
    const fw_fobj_attr_t *desc;

    if(0 != inst_check(inst)){
        return -1;
    }

    desc = inst_attr(inst, attr);
    if(desc == NULL){
        return -1;
    }

    return desc->get ? desc->get(inst->data, buf, size) : -1;
}

int fw_fobj_inst_proc(void *obj, const char *proc, int ac, char **av, char *buf, int size)
{
    fw_fobj_inst_t *inst = (fw_fobj_inst_t *)obj;
    const fw_fobj_proc_t *desc;

    if(0 != inst_check(inst)){
        return -1;
    }

    desc = inst_proc(inst, proc);
    if(desc == NULL){
        return -1;
    }

    return desc->proc ? desc->proc(inst->data, ac, av, buf, size) : -1;
}

