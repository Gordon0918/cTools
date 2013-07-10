#ifndef __G_THREAD_H__
#define __G_THREAD_H__
#ifdef __cplusplus
extern "C"{
#endif

#include <pthread.h>
#include "gspin_lock.h"

typedef struct __gthread_mutex_t
{
    gsmp_spinlock_t glock;
    pthread_mutex_t mtx;
}gthread_mutex_t;


static inline int gthread_cond_wait(pthread_cond_t *cond, 
                                        gthread_mutex_t *mtx)
{
    return pthread_cond_wait(cond, &mtx->mtx);
}

static inline int gthread_mutex_lock(gthread_mutex_t *mtx)
{
    int ret;
    /*disable thread schedule*/
    ret = pthread_mutex_lock(&mtx->mtx);
    if(0 != ret)
        return ret;
    /*wait*/
    gsmp_spin_lock(&mtx->glock);

    return ret;    
}

static inline void gthread_mutex_unlock(gthread_mutex_t *mtx)
{
    gsmp_spin_unlock(&mtx->glock);
    pthread_mutex_unlock(&mtx->mtx);
}


static inline int gthread_mutex_init(gthread_mutex_t *mtx, const pthread_mutexattr_t *attr)
{
    gsmp_spin_lock_init(&mtx->glock);
    return pthread_mutex_init(&mtx->mtx, attr);
}

static inline int gthread_mutex_destroy(gthread_mutex_t *mtx)
{
    return pthread_mutex_destroy(&mtx->mtx);
}


#ifdef __cplusplus
}
#endif
#endif /*__G_THREAD_H__*/


