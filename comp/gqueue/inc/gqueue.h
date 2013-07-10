/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gqueue.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _GQUEUE_INC_
#define _GQUEUE_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GQUEUE_INVALID      ((gqueue_t) 0)
#define GQUEUE_WAIT_NO      (0)
#define GQUEUE_WAIT_FOREVER (-1)

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef void * gqueue_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
gqueue_t gqueue_create(int bsize, int bnum);
int gqueue_destroy(gqueue_t queue);

int gqueue_isempty(gqueue_t queue);

/*Get one node from free queue and copy buf to free queue node buffer, 
   then list into the mq_head  queue*/
int gqueue_send(gqueue_t queue, void *buf, int size, int tout);
/*Get one node from mq_head queue and copy the node buffer to buf */
int gqueue_recv(gqueue_t queue, void *buf, int size, int tout);
/*use func to process all node in the mq_head queue*/
int gqueue_flush(gqueue_t gqueue, void (*func)(void *));
/*get a free queue node*/
void * gqueue_msg_alloc(gqueue_t queue, int size, int tout);
/*list the node into free queue*/
void   gqueue_msg_free(gqueue_t queue, void *);
/*list the node into mq_head queue*/
int    gqueue_msg_send(gqueue_t queue, void *msg);
/*get one node from mq_head queue*/
int    gqueue_msg_recv(gqueue_t queue, void **msg, int tout);

#ifdef _cplusplus
}
#endif

#endif /* _GQUEUE_INC_ */

/* End of gqueue.h */

