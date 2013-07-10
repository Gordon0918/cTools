/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gipc.h
 * 
 * Brief:
 *      geo ipc module header file
 * 
 * History:
 *      05/06/2012  create dinglixing
 *****************************************************************************/

#ifndef _GIPC_INC_
#define _GIPC_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GIPC_MSG_ID_MIN     (16)
#define GIPC_MSG_ID_MAX     (32)
#define GIPC_MSG_LEN        (sizeof(long int) * 4)
#define GIPC_BUF_INVALID    ((gipc_buf_t)0)

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef long int  gipc_id_t;

typedef struct _gipc_msg_t {
    gipc_id_t       dst;                // dst id 
    gipc_id_t       src;                // src id
    unsigned char   msg[GIPC_MSG_LEN];  // message content
}gipc_msg_t;

typedef long int  gipc_buf_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int gipc_init(void);
int gipc_deinit(void);

int gipc_msg_send(gipc_msg_t *msg);
int gipc_msg_wait(gipc_msg_t *msg);

gipc_buf_t gipc_buf_alloc(int size);
void *     gipc_buf_addr(gipc_buf_t buf);
void       gipc_buf_free(gipc_buf_t buf);

#ifdef _cplusplus
}
#endif

#endif /* _GIPC_INC_ */

/* End of gipc.h */

