/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gclock.h
 *
 * Brief:
 *      geo ipc module header file
 *
 * History:
 *      05/06/2012  create xing
 *****************************************************************************/

#ifndef _GCLOCK_INC_
#define _GCLOCK_INC_

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
typedef struct _gprof_t {
    const char         *module_name;
    unsigned int        clock_bgn;
    unsigned long long  clock_sum;
    struct _gprof_t    *parent;
    struct _gprof_t    *child;
    struct _gprof_t    *siblings;
}gprof_t;

#if PROF_ENABLE
#define GPROF_START(gprof) gprof_start(gprof)
#define GPROF_END(gprof)   gprof_end(gprof)   
#else
#define  _GPROF_VOID_CAST 

#define GPROF_START(gprof)  _GPROF_VOID_CAST
#define GPROF_END(gprof)    _GPROF_VOID_CAST
#endif

#if 0  //when PROF_ENABLE is not defined, the Parameter gprof is also not defined
#define _GPROF_VOID_CAST 

#define GPROF_START(gprof) ((!PROF_ENABLE) ? _GPROF_VOID_CAST : gprof_start(gprof))
#define GPROF_END(gprof) ((!PROF_ENABLE) ? _GPROF_VOID_CAST : gprof_end(gprof))
#endif
/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/

/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
typedef int (*usr_proc)(gprof_t *parent, gprof_t *prof, void *udata);
void gprof_init(gprof_t *gprof, const char *name);
void gprof_addchild(gprof_t *parent, gprof_t *child);
void gprof_scan(gprof_t *root, usr_proc user_proc, void *udata);
void gprof_start(gprof_t *gprof);
void gprof_end(gprof_t *gprof);

#ifdef _cplusplus
}
#endif

#endif /* _GCLOCK_INC_ */

/* End of gclock.h */

