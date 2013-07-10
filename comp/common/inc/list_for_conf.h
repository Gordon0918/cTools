#ifndef __LIST__H
#define __LIST__H

#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif    /* __cplusplus */

#if defined(_WIN32) || defined(WIN32)
#define __INLINE__
#else
#define __INLINE__ inline
#endif

#define _CHECK_LIST                1
/*
 * convert from a list's address to another data-type's address.
 * type is the structure's type. ptr is the address of a list. and mm is a field in this
 * structure and whose type is struct list_head.
 */
#define list_entry(type, mm, ptr) ((type *)( (char *)ptr - (unsigned long)&((type *)0)->mm))
#define struct_offset(type, mm)      ((long)&((type *)0)->mm)

struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};

#if _CHECK_LIST == 1
static __INLINE__ int __list_is_valid(struct list_head *h);
#endif    /* _CHECK_LIST */

static __INLINE__ void __list_init(struct list_head *l)
{
    l->prev = l->next = l;
}

static __INLINE__ int __list_empty(struct list_head *h)
{
    return h->next == h;
}

/* add to the head of list
 */
static __INLINE__ void __list_add(struct list_head *n, struct list_head *h)
{
    struct list_head *t = h->next;

    h->next = n;    /* [h] <-> n <-> t(h->next) */
    n->next = t;
    t->prev = n;
    n->prev = h;

#if _CHECK_LIST == 1
    __list_is_valid(h);
#endif    /* _DEBUG */

}

/* add to the tail of list
 */
static __INLINE__ void __list_add_tail(struct list_head *n, struct list_head *h)
{
    struct list_head *t = h->prev;

    t->next = n;    /* t(h->prev) <-> n <-> [h] */
    n->next = h;
    h->prev = n;
    n->prev = t;

#if _CHECK_LIST == 1
    __list_is_valid(h);
#endif    /* _DEBUG */

}

/* merge list to the head of another list(h1 --> head of h2) */
static __INLINE__ void __list_merge(struct list_head *h1, struct list_head *h2)
{
    struct list_head *f1 = h1->next;
    struct list_head *l1 = h1->prev;
    struct list_head *f2 = h2->next;

    if (__list_empty(h1))
        return;

    l1->next = f2;
    f2->prev = l1;

    h2->next = f1;
    f1->prev = h2;

#if _CHECK_LIST == 1
    __list_is_valid(h2);
#endif    /* _DEBUG */

}

static __INLINE__ void __list_merge_tail(struct list_head *h1, struct list_head *h2)
{
    struct list_head *f1 = h1->next;
    struct list_head *l1 = h1->prev;
    struct list_head *l2 = h2->prev;

    if (__list_empty(h1))
        return;

    l2->next = f1;
    f1->prev = l2;

    h2->prev = l1;
    l1->next = h2;

#if _CHECK_LIST == 1
    __list_is_valid(h2);
#endif    /* _DEBUG */

}

static __INLINE__ void __list_del(struct list_head *e)
{
    struct list_head *p = e->prev;
    struct list_head *n = e->next;

    p->next = n;
    n->prev = p;

#if _CHECK_LIST == 1
    __list_is_valid(n);
#endif    /* _DEBUG */

}

static __INLINE__ void __list_del_init(struct list_head *e)
{
    struct list_head *p = e->prev;
    struct list_head *n = e->next;

    p->next = n;
    n->prev = p;

#if _CHECK_LIST == 1
    __list_is_valid(n);
#endif    /* _DEBUG */

    e->prev = e->next = e;
}

#if _CHECK_LIST == 1
static __INLINE__ int __list_is_valid(struct list_head *h)
{
    struct list_head    *p = h, *e, *ha[512];
    int        i = 0;

    memset((void *)ha, 0, sizeof(ha));

    ha[i] = h;
    e = h->next;
    while (e != h) {
        ha[ (i += 1) % 512 ] = e;
        if (e->prev != p) {
            ASSERT(0);
            return -1;
        }

        if (p->next != e) {
            ASSERT(0);
            return -1;
        }

        p = e;
        e = e->next;
    }

    /* now e == h, p = last one */
    if (e->prev != p) {
        ASSERT(0);
        return -1;
    }

    if (p->next != e) {
        ASSERT(0);
        return -1;
    }    

    return 0;
}
#endif    /* _CHECK_LIST */

#ifdef __cplusplus
}
#endif    /* __cplusplus */

#endif    /* __LIST__H */
