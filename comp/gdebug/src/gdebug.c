/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *
 * Brief:
 *
 * History:
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <gthread.h>

//#include "gdebug.h"
/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/

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
#if 0
#define mask(x)  ((unsigned long)(x)  & ~0x1f)

void stack_dump(void)
{   unsigned long *stack_top;
    unsigned long *stack_bot;
    unsigned long *lr;
    int           deep = 0;
    int           end  = 0;
    stack_top = (unsigned long *) &stack_top;
    stack_bot = stack_top + 2;

    printf("\nStack Dump:\n\n");
    while(!end){
        //get stack bottom
        while(1){
            unsigned long sa;
            unsigned long sv;
            sa = (unsigned long)stack_bot;
            sv = *stack_bot;
            if(sa + 0x10 == sv){
                lr        = stack_bot + 1;
                stack_bot = stack_bot + 2;
                break;
            }
            if(sa - 0x10 == sv){
                lr        = NULL;
                stack_bot = stack_bot + 2;
                end       = 1;
                printf("\nGo main!\n");
                break;
            }
            stack_bot++;
        }
        printf("Indeep: %d\n", deep++);
        printf("SP TOP: %p\n", stack_top);
        printf("SP BOT: %p\n", stack_bot);
        if(lr){
            printf("LR: %p\n", *lr);
        }else{
            printf("LR: NV\n");
        }
        while(stack_top < stack_bot){
                unsigned int *s1, *s2;
                s1 = (unsigned int *) stack_top;
                s2 = s1 + 1;
                printf("Stack(%p) : [0x%.8x, 0x%.8x] 0x%.16lx\n", stack_top, *s1, *s2, *stack_top);
                stack_top++;
        }
    }
}

static
void mem_dump(uint32_t *m)
{
    int i;

    for(i = 0; i < 256; i++)
        printf("mem(%.2d): 0x%.8x\n", i, m[i]);
}
#endif

int gpthread_mutex_lock(pthread_mutex_t *lock, const char *file, const char *func, int line)
{
 //   pthread_mutex_t lock2 = *lock;
 //   static int ecnt = 0;
 //   int ret;

    #if 0
    if(lock2.__data.__lock == 0 && lock2.__data.__owner > 0){
        printf("lock: %d\n", lock2.__data.__lock);
        printf("owner: %d\n", lock2.__data.__owner);
        //mem_dump(&lock2);
        printf("%s: %s: %d\r\n", file, func, line);
        //stack_dump();
   //     ecnt++;
    }
    if(lock->__data.__lock != 0){
        printf("stack: %p\n", &plock);
        printf("addr: %p\n", lock);

        printf("lock: %d\n", lock->__data.__lock);
        printf("owner: %d\n", lock->__data.__owner);
        lock->__data.__lock = 0;
    }

    ret = pthread_mutex_lock(lock);
    if(ecnt){
        printf("ecnt: %d\n", ecnt);
        printf("lock: %p\n", lock);
        ecnt = 0;
    }
    #endif



    return pthread_mutex_lock(lock);
}

int gpthread_mutex_unlock(pthread_mutex_t *lock)
{
    return pthread_mutex_unlock(lock);
}

/* End of gdr.c */