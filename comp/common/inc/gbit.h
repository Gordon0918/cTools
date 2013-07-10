/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gbit.h
 *
 * Brief:
 *      geo ipc module header file
 *
 * History:
 *      23/08/2012  create xing
 *****************************************************************************/

#ifndef _GBIT_INC_
#define _GBIT_INC_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdint.h>

/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/

#define _gbit_foreach(blk,len,bitid)\
    for(bitid = 0;bitid < (len << 3);bitid++)

#define gbit_foreach(pblk,bitid)\
    for(bitid=0; bitid<(sizeof(typeof(*(pblk)))<<3);bitid++)


static inline void _gbit_reverse(void *blk, int len){
    unsigned char *p = (unsigned char *)blk;
    int i;

    for(i=0; i<len; i++){
        *p = ~(*p);
        p++;
    }
}

static inline void _gbit_and(void *blk1, void *blk2, int len){
    unsigned char *p1 = (unsigned char *)blk1;
    unsigned char *p2 = (unsigned char *)blk2;
    int i;

    for(i=0; i<len; i++){
        *p1 &= *p2;
        p1++;p2++;
    }
}

static inline void _gbit_or(void *blk1, void *blk2, int len){
    unsigned char *p1 = (unsigned char *)blk1;
    unsigned char *p2 = (unsigned char *)blk2;
    int i;

    for(i=0; i<len; i++){
        *p1 |= *p2;
        p1++;p2++;
    }
}

static inline void _gbit_xor(void *blk1, void *blk2, int len)
{
    unsigned char *p1 = (unsigned char *)blk1;
    unsigned char *p2 = (unsigned char *)blk2;
    int i;

    for(i=0; i<len; i++){
        *p1 ^= *p2;
        p1++;p2++;
    }
}


static inline void _gbit_clr(void *blk, int len){
    int i;
    unsigned char *p = (unsigned char *)blk;

    for(i=0; i<len; i++){
        p[i] = 0;
    }
}

static inline int _gbit_is_zero(void *blk, int len){
    int i;
    unsigned char *p =  (unsigned char *)blk;

    for(i=0; i<len; i++){
        if(p[i] != 0) break;
    }
    if(len == i) return 1;
    return 0;
}

static inline int _gbit_test(void *blk, int len, int id){
    unsigned char *p = (unsigned char *)blk;
    if((id>>3) >= len) return 0;
    return (p[id>>3] & (0x80 >> ((id)&7)));
}

static inline void _gbit_set(void *blk, int len, int idx){
    if(((idx) >> 3) >= len)
        return;
    ((unsigned char *)(blk))[(idx) >> 3] |= (0x80 >> ((idx)&7));
}

static inline void _gbit_unset(void *pblk, int len, int idx){

    if(((idx) >> 3) >= len)
        return;
    ((unsigned char *)(pblk))[(idx) >> 3] &= ~(0x80 >> ((idx)&7));
}


#define gbit_test(pblk, id) _gbit_test(pblk, sizeof(typeof(*(pblk))),id)

#define gbit_set(pblk, idx) \
            _gbit_set(pblk, sizeof(typeof(*(pblk))), idx)

#define gbit_unset(pblk, idx)\
    _gbit_unset(pblk, sizeof(typeof(*(pblk))), idx)

#define gbit_reverse(pblk)\
    _gbit_reverse((pblk), sizeof(typeof(*(pblk))))

#define gbit_and(pblk1, pblk2)\
    _gbit_and((pblk1), (pblk2), sizeof(typeof(*(pblk1))))

#define gbit_or(pblk1,pblk2)\
    _gbit_or((pblk1), (pblk2), sizeof(typeof(*(pblk1))))

#define gbit_clr(pblk)  _gbit_clr((pblk), sizeof(typeof(*(pblk))))

#define gbit_zero(pblk)    gbit_clr(pblk)

#define gbit_is_zero(pblk) _gbit_is_zero((pblk), sizeof(typeof(*(pblk))))

extern void _gbit_print(void *blk, int len);
#define gbit_print(pblk) _gbit_print(pblk, sizeof(typeof(*(pblk))))

#define gbit_xor(pblk1,pblk2) \
    _gbit_xor(pblk1,pblk2,sizeof(typeof(*(pblk1))))



#ifdef _cplusplus
}
#endif

#endif /* _BIT_INC_ */
