/*
swd.c
software watch dog
mickey create
2012.10.8
*/
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gthread.h>
#include <stdint.h>
#include "gutils.h"
#include "list.h"
#include "ghash.h"
#include "swd.h"

#define HASHTBL_CAPST   1024

typedef struct _swd_t{
    char name[32];
    int cur_cnt;
    int maxcnt;
    int enable;
    swdhnd hnd;
    swd_func swdfun;
}swd_t;

   // swd_t *pswd = (swd_t *)arg;


static void *swd_tbl;


int swd_init(){
    swd_tbl = ghash_tbl_create(HASHTBL_CAPST, GHASH_ALG_AUTO);
    if(NULL == swd_tbl) return -1;
    return 0;
}

static int swd_free(void *uarg, void *t, ghash_node_t *node){
    swd_t *swd_node = (swd_t *)node->udata;

    free(swd_node);

    return 0;
}

static void destroy_swd(){
    ghash_node_foreach_del(swd_tbl, swd_free, NULL);
    if(0 == ghash_tbl_delete(swd_tbl)){
        swd_tbl = NULL;
    } else {
        return ;
    }
    return ;
}


void swd_deinit(){
    destroy_swd();
}

static int swd_print(void *arg)
{
  //  swd_t *pswd = (swd_t *)arg;

    if(!arg)
        return -1;

    #if 0
    printf("swd name is %s\r\n",pswd->name);
    printf("swd cur cnt is %d\r\n",pswd->cur_cnt);
    printf("swd max cnt is %d\r\n",pswd->maxcnt);
    printf("swd enable is %d\r\n",pswd->enable);
    printf("swd handle is %p\r\n",pswd->hnd);
    #endif

    return 0;
}


static swdhnd _new_swd(const char *name ,int second, swd_func f, swdhnd hnd){
    swd_t *pswd = NULL;

    pswd = (swd_t *)malloc(galign(sizeof(swd_t),8));
    if(!pswd) return INVALID_HND;

    memset(pswd, 0, sizeof(swd_t));

    pswd->maxcnt = second;
    if(f)
        pswd->swdfun = f;
    else
        pswd->swdfun = swd_print;
    if(hnd)
        pswd->hnd = hnd;
    else
        pswd->hnd = pswd;
    if(name)
        snprintf(pswd->name, sizeof(pswd->name), "%s",name);

    if(-1 == ghash_node_add(swd_tbl, sizeof(swdhnd), &pswd->hnd, (long)pswd))
    {
        free(pswd);
        return INVALID_HND;
    }

    return (swdhnd)pswd->hnd;
}

swdhnd t_new_swd(const char *name,int second, swd_func f){
    pthread_t tid = pthread_self();
    char fullname[32] = {0};

    if(name)
        snprintf(fullname, sizeof(fullname),"%s-%lu",name,tid);
    else
        snprintf(fullname, sizeof(fullname),"Tid-%lu",tid);

    return _new_swd(fullname, second, f, (swdhnd)tid);
}

swdhnd new_swd(const char *name, int second, swd_func f){
    return _new_swd(name, second, f, INVALID_HND);
}

static int swd_setenable(swdhnd hnd, int swt)
{
    swd_t *pswd = NULL;
    ghash_node_t *pnode = NULL;

    if(-1 == ghash_hold_get(swd_tbl, sizeof(hnd), &hnd, &pnode))
    {
        return -1;
    }

    pswd = (swd_t *)pnode->udata;

    pswd->enable = swt;
    pswd->cur_cnt = 0;

    ghash_put_get(swd_tbl, pnode);

    return 0;
}

int swd_enable(swdhnd hnd){
    return swd_setenable(hnd, 1);
}

int t_swd_enable(){
    return swd_setenable((swdhnd)pthread_self(), 1);
}

int swd_disable(swdhnd hnd){
    return swd_setenable(hnd, 0);
}
int t_swd_disable(){
    return swd_setenable((swdhnd)pthread_self(), 0);
}

int swd_feed(swdhnd hnd){
    swd_t *pswd = NULL;
    ghash_node_t *pnode = NULL;

    if(-1 == ghash_hold_get(swd_tbl, sizeof(hnd), &hnd, &pnode))
    {
        return -1;
    }

    pswd = (swd_t *)pnode->udata;

    pswd->cur_cnt = 0;

    ghash_put_get(swd_tbl, pnode);

    return 0;
}

int t_swd_feed(){
    return swd_feed((swdhnd)pthread_self());
}

void swd_delete(swdhnd hnd){
    swd_t *pswd =  NULL;

    ghash_node_del(swd_tbl, sizeof(swdhnd), &hnd, (long *)&pswd);

    if(pswd)
        free(pswd);

}

void t_swd_delete(){
    swd_delete((swdhnd)pthread_self());
}

static int swd_proc(void *uarg, void *t, ghash_node_t *node){
    swd_t *pswd =  (swd_t *)node->udata;

    if(!pswd->enable) return -1;

    pswd->cur_cnt ++;

    if(pswd->cur_cnt >= pswd->maxcnt){
        if(pswd->swdfun)
            pswd->swdfun(pswd);
    }
    return 0;

}

void swd_trigger(void *arg, int arg_len){
    (void)ghash_node_traverse(swd_tbl, swd_proc, NULL);
}


#ifdef __cplusplus
}
#endif

