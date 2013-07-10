/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gqueue.c
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gthread.h"
#include <sys/time.h>
#include <assert.h>

#include <gqueue.h>
#include "list.h"

/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GQUEUE_ALIGN(value, align) \
    (((value)+(align-1)) & (~(align -1)))

#define GQUEUE_OFFSET(addr, off, type)  \
    ((type *)(((unsigned long)(addr)) + (off)))

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _queue_blk_t{
    struct list_head    list;
    char                buf[0];
}queue_blk_t;

typedef struct queue_hdr_t{
    gthread_mutex_t     mq_lock;
    pthread_cond_t      mq_fcond;
    pthread_cond_t      mq_wcond;
    int                 mq_size;
    int                 mq_max;
    struct list_head    mq_head;
    struct list_head    mq_free;
}queue_hdr_t;

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
* Global function
******************************************************************************/
gqueue_t gqueue_create(int bsize, int bnum)
{
    queue_hdr_t *gq;
    void *blk_pool;
    int i;
    int hsize;
    int msize;

    if(0 == bsize || 0 == bnum) {
        return GQUEUE_INVALID;
    }

    hsize = GQUEUE_ALIGN(sizeof(queue_hdr_t), 16);
    msize = GQUEUE_ALIGN(bsize + sizeof(queue_blk_t), 8);

    gq = (queue_hdr_t *)malloc(hsize + msize * bnum);
    if (NULL == gq) {
        return GQUEUE_INVALID;
    }

    if (0 != gthread_mutex_init(&gq->mq_lock, NULL)) {
        goto DO_ERR_RET;
    }

    if (0 != pthread_cond_init(&gq->mq_fcond, NULL)) {
        goto DO_ERR_RET;
    }

    if (0 != pthread_cond_init(&gq->mq_wcond, NULL)) {
        goto DO_ERR_RET;
    }

    gq->mq_max  = bnum;
    gq->mq_size = msize - sizeof(queue_blk_t);

    INIT_LIST_HEAD(&gq->mq_head);
    INIT_LIST_HEAD(&gq->mq_free);

    blk_pool = GQUEUE_OFFSET(gq, hsize, void);

    for(i = 0; i < bnum; i++){
        queue_blk_t *blk;

        blk = GQUEUE_OFFSET(blk_pool, i * msize, queue_blk_t);
        list_add(&blk->list, &gq->mq_free);
    }

    return (gqueue_t)gq;

DO_ERR_RET:
    free(gq);
    return GQUEUE_INVALID;
}

int gqueue_destroy(gqueue_t gqueue)
{
    queue_hdr_t *gq = (queue_hdr_t *)gqueue;

    if (NULL == gqueue) {
        return -1;
    }

    if (0 != pthread_cond_destroy(&gq->mq_fcond)) {
        return -1;
    }

    if (0 != pthread_cond_destroy(&gq->mq_wcond)) {
        return -1;
    }

    if (0 != gthread_mutex_destroy(&gq->mq_lock)) {
        return -1;
    }

    free(gq);

    return 0;
}

int gqueue_isempty(gqueue_t gqueue)
{
    queue_hdr_t  *gq = gqueue;

    return list_empty(&gq->mq_head);
}

int gqueue_isfull(gqueue_t gqueue)
{
    queue_hdr_t  *gq = gqueue;

    return list_empty(&gq->mq_free);
}

int gqueue_send(gqueue_t gqueue, void *buf, int size, int tout)
{       
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk;
    int retval = 0;


    if(NULL == gqueue)
        return -1;

    if(size > gq->mq_size){
        return -1;
    }

    if(0 != gthread_mutex_lock(&gq->mq_lock)){
        return -1;
    }

    while(list_empty(&gq->mq_free)){
        if(tout == GQUEUE_WAIT_NO){
            retval = -1;
            goto DO_RETURE;
        } else {
                if(0 != gthread_cond_wait(&gq->mq_fcond,&gq->mq_lock)){
                retval = -1;
                goto DO_RETURE;
            }
        }
    }

    gblk = list_first_entry(&gq->mq_free, queue_blk_t, list);

    list_del(&gblk->list);

    memcpy(gblk->buf, buf, size);

    list_add_tail(&gblk->list, &gq->mq_head);
    pthread_cond_signal(&gq->mq_wcond);

DO_RETURE:
    gthread_mutex_lock(&gq->mq_lock);

    return retval;
}

int gqueue_recv(gqueue_t gqueue, void *buf, int size, int tout)
{
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk;
    int retval = 0;

    if (NULL == gqueue) {//yes! Because it is empty
        printf("0\n");
        return -1;
    }

    if (size > gq->mq_size) {
        return -2;
    }

    if (0 != gthread_mutex_lock(&gq->mq_lock)) {
        return -3;
    }


    while(list_empty(&gq->mq_head)){
        if(tout == GQUEUE_WAIT_NO){
            retval = -1;
            goto DO_RETURE;
        } else{
            if(0 != gthread_cond_wait(&gq->mq_wcond,
                                      &gq->mq_lock)){
                retval = -1;
                goto DO_RETURE;
            }
        }
    }

    gblk = list_first_entry(&gq->mq_head, queue_blk_t, list);
    list_del(&gblk->list);

    memcpy(buf, gblk->buf, size);

    list_add_tail(&gblk->list, &gq->mq_free);
    pthread_cond_signal(&gq->mq_fcond);

DO_RETURE:
    gthread_mutex_lock(&gq->mq_lock);
    return retval;
}

int gqueue_flush(gqueue_t gqueue, void (*func)(void *))
{
    queue_hdr_t  *gq = gqueue;

    struct list_head *iter, *next;

    if (NULL == gqueue) {
        return -1;
    }

    if (0 != gthread_mutex_lock(&gq->mq_lock)) {
        return -1;
    }
   
    list_for_each_safe(iter, next, &gq->mq_head){
        queue_blk_t  *gblk;

        gblk = list_entry(iter, queue_blk_t, list);
        if(func){
            func(gblk->buf);
        }
        list_move_tail(iter, &gq->mq_free);
    }

    gthread_mutex_lock(&gq->mq_lock);
    return 0;
}

void * gqueue_msg_alloc(gqueue_t gqueue, int size, int tout)
{
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk = NULL;

    if(NULL == gqueue)
        return NULL;

    if(size > gq->mq_size){
        return NULL;
    }

    if(0 != gthread_mutex_lock(&gq->mq_lock)){
        return NULL;
    }

    while(list_empty(&gq->mq_free)){
        if(tout == GQUEUE_WAIT_NO){
            goto DO_RETURN;
        } else {
            if(0 != gthread_cond_wait(&gq->mq_fcond,&gq->mq_lock)){
                goto DO_RETURN;
            }
        }
    }

    gblk = list_first_entry(&gq->mq_free, queue_blk_t, list);
    list_del(&gblk->list);


DO_RETURN:

    gthread_mutex_lock(&gq->mq_lock);
    if(gblk){
        return gblk->buf;
    }else{
        return NULL;
    }
}

void   gqueue_msg_free(gqueue_t gqueue, void *msg)
{
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk;

    if(NULL == gqueue)
        return;

    gblk = container_of(msg, queue_blk_t, buf);

    if(0 != gthread_mutex_lock(&gq->mq_lock)){
        return;
    }

    list_add_tail(&gblk->list, &gq->mq_free);

    pthread_cond_signal(&gq->mq_fcond);
    gthread_mutex_lock(&gq->mq_lock);
}

int gqueue_msg_send(gqueue_t gqueue, void *msg)
{
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk;
    int retval = 0;

    if(NULL == gqueue)
        return -1;

    gblk = container_of(msg, queue_blk_t, buf);

    if(0 != gthread_mutex_lock(&gq->mq_lock)){
        return -1;
    }

    list_add_tail(&gblk->list, &gq->mq_head);

    pthread_cond_signal(&gq->mq_wcond);
    gthread_mutex_lock(&gq->mq_lock);

    return retval;
}

int gqueue_msg_recv(gqueue_t gqueue, void **msg, int tout)
{
    queue_hdr_t  *gq = gqueue;
    queue_blk_t  *gblk;
    int retval = 0;

    if (NULL == gqueue) {
        return -1;
    }

    if (0 != gthread_mutex_lock(&gq->mq_lock)) {
        return -3;
    }

    while(list_empty(&gq->mq_head)){
        if(tout == GQUEUE_WAIT_NO){
            retval = -1;
            goto DO_RETURE;
        } else{
            if(0 != gthread_cond_wait(&gq->mq_wcond,
                                      &gq->mq_lock)){
                retval = -1;
                goto DO_RETURE;
            }
        }
    }

    gblk = list_first_entry(&gq->mq_head, queue_blk_t, list);
    list_del(&gblk->list);

    *msg = gblk->buf;

    pthread_cond_signal(&gq->mq_fcond);

DO_RETURE:
    gthread_mutex_lock(&gq->mq_lock);
    return retval;
}

/* End of gqueue.c */

