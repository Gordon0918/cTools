/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     gclock.c
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
#include <stdint.h>
#include <assert.h>

#include <gclock.h>
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
extern void xlr_fast_syscall_c0_count(unsigned int *counter);

/******************************************************************************
 * Local function define                                                     *
 *****************************************************************************/
static unsigned int gprof_get_clock(void)
{
    unsigned int clock;
//#ifndef HW_PC
//    xlr_fast_syscall_c0_count(&clock);
//#endif
    clock = 0;
    return clock;
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
void gprof_init(gprof_t *gprof, const char *name)
{
    gprof->module_name = name;
    gprof->clock_bgn   = 0;
    gprof->clock_sum   = 0;
    gprof->parent      = NULL;
    gprof->child       = NULL;
    gprof->siblings    = NULL;
}

void gprof_addchild(gprof_t *parent, gprof_t *child)
{
    if(parent == NULL){
        return;
    } else {
        child->parent   = parent;
        child->siblings = parent->child;
        parent->child   = child;
    }
}

void gprof_start(gprof_t *gprof)
{
    gprof->clock_bgn = gprof_get_clock();
}

void gprof_end(gprof_t *gprof)
{
    unsigned int clock_used;
    clock_used = gprof_get_clock() - gprof->clock_bgn;
    gprof->clock_sum += clock_used;
}

void gprof_traverse(gprof_t *root, usr_proc user_proc, void *udata)
{
    if(root == NULL){
        return ;
    }
    
    gprof_traverse(root->child, user_proc, udata);
    if(root->parent != NULL){
        user_proc(root->parent, root, udata);
    }
    gprof_traverse(root->siblings, user_proc, udata);
}

void gprof_scan(gprof_t *root, usr_proc user_proc, void *udata)
{  
    gprof_t *pos;

    if(root->child == NULL){
        return;
    }
    pos = root->child;
    
#if 1 // traverse all tree
    gprof_traverse(pos, user_proc, udata);
#else // only traverse cur level tree;
    for(pos = root->child; pos->siblings != NULL; pos = pos->siblings)
    {
        user_proc(pos->parent, pos, udata);
    }

#endif
}
/* End of gclock.c */

