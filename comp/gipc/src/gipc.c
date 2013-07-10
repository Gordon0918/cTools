/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     gipc.c
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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>

#include <gipc.h>

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define FILE_NAME_MSGQUEUE  "/"
#define FILE_NAME_SHM       "/"

#define IPC_MSG_MAX             (10240)

#define IPC_SHM_NR              (1)
#define IPC_SHM_0_TYPE          (0)
#define IPC_SHM_0_NUM           (4096)
#define IPC_SHM_0_BLK_SIZE      (2048)
#define IPC_SHM_0_SIZE          (IPC_SHM_0_NUM * IPC_SHM_0_BLK_SIZE)
#define IPC_SHM_0_Q             (GIPC_MSG_ID_MAX + 0)
#define IPC_BUF_MAX             (IPC_SHM_0_NUM)

#define IPC_BUF_MAGIC           (0x3C000000)
#define IPC_BUF_MMASK           (0xFF000000)
#define IPC_BUF_TMASK           (0x00FF0000)
#define IPC_BUF_IMASK           (0x0000FFFF)

#define IPC_BUF_MK_ID(type,idx) (IPC_BUF_MAGIC|((type) << 16)|(idx))
#define IPC_BUF_MAGIC_GET(b)    ((b)&IPC_BUF_MMASK)
#define IPC_BUF_TYPE_GET(b)     (((b)&IPC_BUF_TMASK) >> 16)
#define IPC_BUF_IDX_GET(b)      ((b)&IPC_BUF_IMASK)

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gipc_buf_msg_t {
    long        mtype;
    gipc_buf_t  mtext;
}gipc_buf_msg_t;

/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/
static int   gipc_msgq_id  = -1;
static int   gipc_shm_id   = -1;
static void *gipc_shm_base = NULL;

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static int gipc_msgq_create(void)
{
    key_t           k1;
    int             msg_id;
    struct msqid_ds msq_set;
    int             ret;

    k1 = ftok(FILE_NAME_MSGQUEUE, 0);
    if (k1 < 0) {
        perror("ftok");
        return -1;
    }

    msg_id = msgget(k1, IPC_CREAT | IPC_EXCL | 0666);
    if (msg_id < 0) {
        perror("msgget");
        if(errno == EEXIST){
            //Todo: delete and reget?
            assert(0);
        }
        return -1;
    }

    ret = msgctl(msg_id, IPC_STAT, &msq_set);
    if (ret < 0) {
        perror("msgctl");
        return -1;
    }

    msq_set.msg_qbytes = sizeof(gipc_msg_t) * (IPC_MSG_MAX + IPC_BUF_MAX);
    msq_set.msg_qnum   = IPC_MSG_MAX + IPC_BUF_MAX;
    ret = msgctl(msg_id, IPC_SET, &msq_set);
    if (ret < 0) {
        perror("msgctl");
        return -1;
    }
    gipc_msgq_id = msg_id;

    return 0;
}

static void gipc_msgq_delete(void)
{
    if(gipc_msgq_id >= 0){
        int ret = msgctl(gipc_msgq_id, IPC_RMID, NULL);
        if(0 == ret){
            gipc_msgq_id = -1;
        }
    }
}

static int gipc_shm_create(void)
{
    key_t k1;
    int shmid;
    void *shmaddr;


    k1 = ftok(FILE_NAME_SHM, 0);
    if (k1 < 0) {
        return -1;
    }

    shmid = shmget(k1, IPC_SHM_0_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid < 0) {
        if(errno == EEXIST){
            //Todo: delete and reget?
            assert(0);
        }
        return -1;
    }

    shmaddr = shmat(shmid, 0, 0);
    if (shmaddr == (void *)(-1)) {
        shmctl(shmid, IPC_RMID, NULL);
        return -1;
    }

    gipc_shm_id     = shmid;
    gipc_shm_base   = shmaddr;

    return 0;
}

static void gipc_shm_delete(void)
{
    if(gipc_shm_base != NULL){
        int ret = shmdt(gipc_shm_base);
        if(0 == ret){
            gipc_shm_base = NULL;
        }
    }

    if(gipc_shm_id >= 0){
        int ret = shmctl(gipc_shm_id, IPC_RMID, NULL);
        if(0 == ret){
            gipc_shm_id = -1;
        }
    }
}

static int gipc_buf_init(void)
{
    int i;

    for(i = 0; i < IPC_SHM_0_NUM; i++){
        gipc_buf_t buf_id = IPC_BUF_MK_ID(IPC_SHM_0_TYPE, i);

        gipc_buf_free(buf_id);
    }

    return 0;
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
int gipc_init(void)
{
    if(0 != gipc_msgq_create()){
        return -1;
    }

    if(0 != gipc_shm_create()){
        gipc_msgq_delete();
        return -1;
    }

    if(0 != gipc_buf_init()){
        gipc_msgq_delete();
        gipc_shm_delete();
        return -1;
    }

    return 0;
}

int gipc_deinit(void)
{
    gipc_msgq_delete();
    gipc_shm_delete();

    return 0;
}

int gipc_msg_send(gipc_msg_t *msg)
{
    //Param check
    return msgsnd(gipc_msgq_id, msg, sizeof(gipc_msg_t) - sizeof(gipc_id_t), 0);
}

int gipc_msg_wait(gipc_msg_t *msg)
{
    int ret;

    //Param check
    ret = msgrcv(gipc_msgq_id, msg, sizeof(gipc_msg_t) - sizeof(gipc_id_t), msg->dst, 0);
    if(ret != sizeof(gipc_msg_t) - sizeof(gipc_id_t)){
        perror("msgrcv");
        return -1;
    }

    return 0;
}

gipc_buf_t gipc_buf_alloc(int size)
{
    gipc_buf_msg_t bmsg;
    //static long long cnt = 0;

    if(size <= IPC_SHM_0_BLK_SIZE){
        bmsg.mtype = IPC_SHM_0_Q;
        bmsg.mtext = GIPC_BUF_INVALID;

        if(sizeof(gipc_buf_t) == msgrcv(gipc_msgq_id, &bmsg,
                                        sizeof(gipc_buf_t),
                                        IPC_SHM_0_Q, IPC_NOWAIT)){
            //printf("gipc_buf_alloc: %llu\n", ++cnt);
            return bmsg.mtext;
        }
    }
    return GIPC_BUF_INVALID;
}

void * gipc_buf_addr(gipc_buf_t buf)
{
    if(IPC_BUF_MAGIC_GET(buf) == IPC_BUF_MAGIC){
        if(IPC_BUF_TYPE_GET(buf) == IPC_SHM_0_TYPE){
            int idx = IPC_BUF_IDX_GET(buf);

            if(idx < IPC_SHM_0_NUM){
                return gipc_shm_base + IPC_SHM_0_BLK_SIZE * idx;
            }
        }
    }
    return NULL;
}

void   gipc_buf_free(gipc_buf_t buf)
{
    if(IPC_BUF_MAGIC_GET(buf) == IPC_BUF_MAGIC){
        if(IPC_BUF_TYPE_GET(buf) == IPC_SHM_0_TYPE){
            if(IPC_BUF_IDX_GET(buf) < IPC_SHM_0_NUM){
                gipc_buf_msg_t bmsg;
                bmsg.mtype = IPC_SHM_0_Q;
                bmsg.mtext = buf;
                msgsnd(gipc_msgq_id, &bmsg, sizeof(gipc_buf_t),0);
            }
        }
    }
}

/* End of gipc.c */

