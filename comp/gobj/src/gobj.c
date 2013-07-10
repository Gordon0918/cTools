/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gobj.c
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/
#if defined HW_XLR732
/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gthread.h>
#include <gobj.h>
#include <glog.h>

/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gobj_blk{
    struct _gobj_blk *ref_next;
    gthread_mutex_t      ref_lock;
    int                  ref_cnt;
    void                *ref_val;
    void               (*ref_destructor)(void *);
}gobj_blk;

/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/
static gthread_mutex_t gblk_lock = PTHREAD_MUTEX_INITIALIZER;
static gobj_blk       *gblk_head = NULL;
static void           *gblk_pool = NULL;
static int             gblk_num  = 0;

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static inline gobj_blk *alloc_blk(void)
{
    gobj_blk * pblk;

    gthread_mutex_lock(&gblk_lock);

    pblk = gblk_head;
    if(NULL != pblk){
        gblk_head = pblk->ref_next;
    }

    gthread_mutex_lock(&gblk_lock);

    return pblk;
}

static inline void free_blk(gobj_blk *blk)
{
    gthread_mutex_lock(&gblk_lock);

    blk->ref_next = gblk_head;
    gblk_head  = blk;

    gthread_mutex_lock(&gblk_lock);
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
int gobj_init(int max)
{
    gobj_blk *pblk, *pnxt;
    int i;

    if(1 > max || NULL != gblk_pool){
        return -1;
    }

    gblk_pool = malloc(max * sizeof(gobj_blk));
    if(NULL == gblk_pool){
        GLOG_PRINT(GLOG_FATAL_ERROR,1, "gblk_pool malloc error\n");
        return -1;
    }

    pblk = (gobj_blk *)gblk_pool;
    pnxt = NULL;
    for(i = 0; i < max; i++, pblk++){
        //printf("add pblk: %p->%p\n", pblk, pnxt);
        pblk->ref_next = pnxt;
        pblk->ref_cnt  = 0;
        pblk->ref_val  = NULL;
        pblk->ref_destructor = NULL;
        gthread_mutex_init(&pblk->ref_lock, NULL);
        gblk_head = pblk;

        pnxt = pblk;
    }
    gblk_num = max;

    return 0;
}

int gobj_deinit(void)
{
    gobj_blk *pblk;
    int i;

    //This is not a safe function!!!

    if(NULL == gblk_pool){
        return -1;
    }

    pblk = (gobj_blk *)gblk_pool;
    for(i = 0; i < gblk_num; i++, pblk++){
        assert(pblk->ref_cnt == 0);
        gthread_mutex_destroy(&pblk->ref_lock);
    }

    free(gblk_pool);
    gblk_pool = NULL;
    gblk_head = NULL;

    return 0;
}

gobj_t gobj_create(void *value, void (*destructor)(void *))
{
    gobj_blk *pblk;

    pblk = alloc_blk();
    if(NULL == pblk){
        return GOBJ_INVALID;
    }

    pblk->ref_cnt = 1;
    pblk->ref_val = value;
    pblk->ref_destructor = destructor;

    return (gobj_t) pblk;
}

gobj_t gobj_dup(gobj_t gobj)
{
    gobj_blk *pblk = (gobj_blk *)gobj;

    if(NULL == pblk){
        return GOBJ_INVALID;
    }

    gthread_mutex_lock(&pblk->ref_lock);
    assert(pblk->ref_cnt != 0);
    pblk->ref_cnt++;
    gthread_mutex_lock(&pblk->ref_lock);

    return (gobj_t) pblk;
}

void *gobj_value(gobj_t gobj)
{
    gobj_blk *pblk = (gobj_blk *)gobj;

    if(NULL == pblk){
        return NULL;
    }

    return pblk->ref_val;
}

void gobj_delete(gobj_t gobj)
{
    gobj_blk *pblk = (gobj_blk *)gobj;
    void (*destructor)(void *) = NULL;
    void *value = NULL;

    if(NULL == pblk){
        return;
    }

    gthread_mutex_lock(&pblk->ref_lock);
    assert(pblk->ref_cnt > 0);

    pblk->ref_cnt--;
    if(pblk->ref_cnt == 0){
        destructor = pblk->ref_destructor;
        value      = pblk->ref_val;
    }
    gthread_mutex_lock(&pblk->ref_lock);

    if(destructor){
        free_blk(pblk);
        destructor(value);
    }

    return;
}

#else
/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gobj.h>
#include <glog.h>
#include <gatomic.h>

/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GOBJ_MAGIC  0x20130117


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gobj_blk_t {
    int    magic;
    int    refcnt;
    void  *value;
    void (*destructor)(void *);
}gobj_blk_t;

/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
int gobj_init(int max)
{
    (void) max; 
    return 0;
}

int gobj_deinit(void)
{
    return 0;
}

gobj_t gobj_create(void *value, void (*destructor)(void *))
{
    gobj_blk_t *obj;

    obj = (gobj_blk_t *)malloc(sizeof(gobj_blk_t));
    if(obj == NULL){
        return GOBJ_INVALID;
    }

    obj->magic  = GOBJ_MAGIC;
    obj->refcnt = 1;
    obj->value  = value;
    obj->destructor = destructor;
    
    return (gobj_t)obj;
}

void gobj_delete(gobj_t gobj)
{
    gobj_blk_t *obj = (gobj_blk_t *)gobj;
    

    if(NULL == obj || GOBJ_MAGIC != obj->magic){
        return;
    }

    if(1 > atomic_dec_fetch(&obj->refcnt)){
        void (*destructor)(void *) = obj->destructor;
        void *value = obj->value;

        obj->magic = 0;
        
        free(obj);
        
        if(destructor && value){
            destructor(value);
        }
    }

    return;
}

gobj_t gobj_dup(gobj_t gobj)
{
    gobj_blk_t *obj = (gobj_blk_t *)gobj;

    if(NULL == obj || GOBJ_MAGIC != obj->magic){
        return GOBJ_INVALID;
    }

    if(1 > atomic_inc_fetch(&obj->refcnt)){
        assert(0);
    }

    return gobj;
}

void *gobj_value(gobj_t gobj)
{
    gobj_blk_t *obj = (gobj_blk_t *)gobj;

    if(NULL == obj || GOBJ_MAGIC != obj->magic){
        return NULL;
    }

    return obj->value;
}
#endif

/* End of gobj.c */

