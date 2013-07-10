/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gatomic.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _GATOMIC_INC_
#define _GATOMIC_INC_

#include <unistd.h>

/******************************************************************************
 * Types & Define                                                             *
 *****************************************************************************/


/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
//function: atomic_lock_take
//      try to get a lock
//param:
//      lock: point to the variable as lock
//      retry: times to try if lock fail
//return:
//       0: get lock
//   other: not get lock
static inline int atomic_lock_take(int *lock, int retry)
{
    while(__sync_lock_test_and_set(lock, 1)){
        if(retry){
            retry --;
            usleep(10);
        }else{
            return -1;
        }
    }

    return 0;
}
static void atomic_lock(int *lock)
{
    while(__sync_lock_test_and_set(lock, 1));
}

//function: atomic_lock_give
//      release a lock
//param:
//      lock: point to the variable as lock
//      retry: times to try if lock fail
//return:
//       0: get lock
//   other: not get lock
static inline void atomic_lock_give(int *lock)
{
    __sync_lock_release(lock);
}

static inline void atomic_unlock(int *lock)
{
    __sync_lock_release(lock);
}

//function: atomic_inc_fetch
//      as (return ++(*a);), but atomic 
//param:
//      atomic: variable to add and fetch
//return:
//      new value 
static inline int atomic_inc_fetch(int *atomic)
{
    return __sync_add_and_fetch(atomic, 1);
}


static inline int atomic_add_fetch(int *atomic, int n)
{
    return __sync_add_and_fetch(atomic, n);
}

static inline int atomic_ladd_fetch(long *atomic, int n)
{
    return __sync_add_and_fetch(atomic, n);
}
static inline int atomic_lladd_fetch(long long *atomic, int n)
{
    return __sync_add_and_fetch(atomic, n);
}

//function: atomic_inc_fetch
//      as (return --(*a);), but atomic 
//param:
//      atomic: variable to dec and fetch
//return:
//      new value 
static inline int atomic_dec_fetch(int *atomic)
{
    return __sync_sub_and_fetch(atomic, 1);
}

static inline int atomic_sub_fetch(int *atomic, int n)
{
    return __sync_sub_and_fetch(atomic, n);
}
static inline int atomic_lsub_fetch(long *atomic, int n)
{
    return __sync_sub_and_fetch(atomic, n);
}

static inline int atomic_llsub_fetch(long long *atomic, int n)
{
    return __sync_sub_and_fetch(atomic, n);
}

//function: atomic_fetch_inc
//      as (return (*a)++;), but atomic 
//param:
//      atomic: variable to fetch and inc
//return:
//      new value 
static inline int atomic_fetch_inc(int *atomic)
{
    return __sync_fetch_and_add(atomic, 1);
}

//function: atomic_fetch_dec
//      as (return (*a)--;), but atomic 
//param:
//      atomic: variable to fetch and dec
//return:
//      new value 
static inline int atomic_fetch_dec(int *atomic)
{
    return __sync_fetch_and_sub(atomic, 1);
}

//function: atomic_test_set
//      as (return ((*a) == cmp) ? (*a) = val, 1 : 0;), but atomic 
//param:
//      atomic: variable to test and set
//      cmp:expect 
//      val: 
//return:
//      0: set nothing
//  other: set ok
static inline int atomic_test_set(int *atomic, int cmp, int val)
{
    return __sync_bool_compare_and_swap(atomic, cmp, val);
}


#endif /* _GATOMIC_INC_ */

/* End of gatomic.h */

