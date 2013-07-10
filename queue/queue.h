
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
typedef struct queue_list_tag
{
    struct queue_list_tag *next;
}queue_list;

enum e_lock_type{
    NO_LOCK,
    SPIN_LOCK,   /*need gatomic support*/
    MUTEX_LOCK   /*need pthread support*/
};

#define queue_member(t, m) \
    ((unsigned long)(&(((t *)0)->m)))
    
#define queue_entity(e, t, m)   \
    ((t *)((unsigned long)(e) -  queue_member(t,m)))


extern void *create_queue(void);
extern void destroy_queue(void *);

static inline void queue_list_init(queue_list *ql)
{
    ql->next = NULL;
}

extern int enqueue(void *, queue_list *);
extern queue_list* dequeue(void *);
extern int queue_num(void *);





#endif /*__QUEUE_H__*/



