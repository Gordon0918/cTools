/*
swd.h
software watch dog
mickey create
2012.10.8
*/

#ifndef __SWD_H__
#define __SWD_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef void* swdhnd;
//typedef int   swdhnd;

#define INVALID_HND (swdhnd)0
//#define SWD_ARRAY

#define SWD_ENABLE


typedef int (*swd_func)(void *arg);


int swd_init();
void swd_trigger(void *arg, int arg_len);
void swd_deinit();
swdhnd new_swd(const char *name ,int second, swd_func f);
void swd_delete(swdhnd hnd);
int swd_enable(swdhnd hnd);
int swd_disable(swdhnd hnd);
int swd_feed(swdhnd hnd);

/*for thread*/
swdhnd t_new_swd(const char *name,int second, swd_func f);
void t_swd_delete();
int t_swd_enable();
int t_swd_disable();
int t_swd_feed();


#ifdef __cplusplus
}
#endif

#endif

