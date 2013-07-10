#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gthread.h>
#include <stdint.h>
#include <assert.h>

#include "list.h"
#include "ghash.h"
#include "mpool.h"

#define GHASH_MAGIC 34598701

typedef uint32_t (*ghash_alg_t)(size_t len, void *key);

typedef struct _ghash_inner_node_t {
    struct list_head list;
    uint8_t          used;
    uint8_t          inwr;
    uint8_t          reserved;
    uint8_t          key_len;
    uint32_t         key_hash;
    uint8_t          key_dat[GHASH_KEY_SIZE_MAX];
    ghash_node_t     data;
}ghash_inner_node_t;

typedef struct _ghash_entry_t {
    int              node_magic;
    int              node_cnt;
    gthread_mutex_t  lock;
    struct list_head node_list;
}ghash_entry_t;

typedef struct _ghash_table_t {
    unsigned int   magic;
    unsigned int   flags;
    unsigned int   node_num;    //not used!
    unsigned int   entry_mask;
    ghash_alg_t    alg_func;
    void          *node_pool;
    ghash_entry_t  entrys[0];
}ghash_table_t;



static uint32_t ghash_alg_js(size_t len, void *key)
{
    uint32_t  hash = 1315423911;
    uint32_t  i    = 0;
    uint8_t  *dat = (uint8_t *)key;

    for(i = 0; i < len; dat++, i++)
    {
        hash ^= ((hash << 5) + (*dat) + (hash >> 2));
    }
    return hash;
}

static uint32_t ghash_alg_elf(size_t len, void *key)
{
    uint32_t hash = 0;
    uint32_t x    = 0;
    uint32_t i    = 0;
    uint8_t  *dat = (uint8_t *)key;

    for(i = 0; i < len; dat++, i++)
    {
        hash = (hash << 4) + (*dat);
        if((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }

    return hash;
}

static inline uint32_t ghash_get_key(ghash_table_t *ghtb, int len, void *dat)
{
    uint32_t key;

    key = ghtb->alg_func(len, dat) & ghtb->entry_mask;

    return key;
}

static inline ghash_inner_node_t *ghash_node_alloc(ghash_table_t *ghtb)
{
    ghash_inner_node_t * node;

    node = mpool_alloc(ghtb->node_pool);
    if(node){
        node->used = 0;
    }

    return node;
}

static inline int  ghash_node_free(ghash_table_t *ghtb, ghash_inner_node_t *node)
{
    assert(node);

    mpool_free(ghtb->node_pool, node);
    return 0;
}

static inline ghash_inner_node_t *ghash_node_find(ghash_entry_t *entry, int key_len, void *key_dat)
{
    ghash_inner_node_t  *find_node = NULL;

    if(entry->node_cnt > 0){
        struct list_head  *iter;

        __list_for_each(iter, &entry->node_list){
            ghash_inner_node_t  *cur_node;

            cur_node = list_entry(iter, ghash_inner_node_t, list);
            if(key_len == cur_node->key_len
               && memcmp(cur_node->key_dat, key_dat, key_len) == 0){
                find_node = cur_node;
                break;
            }
        }
    }
    return find_node;
}

static inline ghash_entry_t *ghash_entry_get_by_id(ghash_table_t *ghtb, uint32_t entry_id)
{
    return &ghtb->entrys[entry_id];
}

static inline ghash_entry_t *ghash_entry_get(ghash_table_t *ghtb, int len, void *dat)
{
    uint32_t        entry_id;
    ghash_entry_t  *entry;


    entry_id = ghtb->alg_func(len, dat) & ghtb->entry_mask;
    entry    = &ghtb->entrys[entry_id];

    return entry;
}

static inline void ghash_entry_lock(ghash_entry_t *entry)
{
    if(entry->node_magic != 34598701){
        printf("%p 1ghash_node_request %u fail! at %s:%d, %s\r\n",&entry->lock, *(((uint32_t *)(&entry->lock))+2),__FILE__, __LINE__,__func__);
        sleep(1);
    }
    gthread_mutex_lock(&entry->lock);
}

static inline void ghash_entry_unlock(ghash_entry_t *entry)
{
    if(entry->node_magic != 34598701){
        printf("%p 2ghash_node_request %u fail! at %s:%d, %s\r\n",&entry->lock, *(((uint32_t *)(&entry->lock))+2),__FILE__, __LINE__,__func__);
        sleep(1);
    }
    gthread_mutex_lock(&entry->lock);
}

void *ghash_tbl_create(int node_max, int alg)
{
    unsigned int    node_num;
    unsigned int    entry_num, entry_id;
    unsigned int    entry_mask;
    ghash_alg_t     alg_func;
    ghash_table_t  *ghtb;
    void           *node_pool;

    if(node_max < 1){
        return NULL;
    }

    node_num   = node_max;
    entry_mask = 0;
    while(node_max){
        entry_mask = (entry_mask << 1) + 1;
        node_max   = node_max & (~entry_mask);
    }
    entry_num  = entry_mask + 1;

    switch(alg){
        case GHASH_ALG_AUTO:
            alg_func = ghash_alg_elf;
            break;
        case GHASH_ALG_JS:
            alg_func = ghash_alg_js;
            break;
        case GHASH_ALG_ELF:
            alg_func = ghash_alg_elf;
            break;
        default:
            alg_func = ghash_alg_elf;
            break;
    }
    ghtb = (ghash_table_t *)malloc(sizeof(ghash_table_t)
                                 + sizeof(ghash_entry_t) * entry_num);
    if(NULL == ghtb){
        return NULL;
    }

    node_pool = mpool_create(sizeof(ghash_inner_node_t), node_num, 0);
    if(NULL == node_pool){
        free(ghtb);
        return NULL;
    }

    for(entry_id = 0; entry_id < entry_num; entry_id++){
        gthread_mutex_init(&ghtb->entrys[entry_id].lock, NULL);
        INIT_LIST_HEAD(&ghtb->entrys[entry_id].node_list);
        ghtb->entrys[entry_id].node_magic = GHASH_MAGIC;
        ghtb->entrys[entry_id].node_cnt = 0;
    }

    ghtb->magic      = GHASH_MAGIC;
    ghtb->flags      = 0;               //Reserved now
    ghtb->node_num   = node_num;
    ghtb->entry_mask = entry_mask;
    ghtb->alg_func   = alg_func;
    ghtb->node_pool  = node_pool;

    return ghtb;
}

int   ghash_tbl_delete(void *t)
{
    unsigned int    entry_num, entry_id;
    ghash_table_t  *ghtb = (ghash_table_t *)t;

    if(t == NULL || ghtb->magic != GHASH_MAGIC){
        return -1;
    }

    if(0 != mpool_delete(ghtb->node_pool)){
        return -1;
    }

    entry_num = ghtb->entry_mask + 1;
    for(entry_id = 0; entry_id < entry_num; entry_id++){
        gthread_mutex_destroy(&ghtb->entrys[entry_id].lock);
    }

    ghtb->magic = 0;

    free(ghtb);

    return 0;
}

int ghash_node_add(void *t, int key_len, void *key_dat, long udata)
{
    ghash_table_t       *ghtb = (ghash_table_t *)t;
    ghash_entry_t       *entry;
    ghash_inner_node_t  *node;

    assert(t);
    assert(key_len > 0 && key_len < GHASH_KEY_SIZE_MAX);
    assert(key_dat);

    node = ghash_node_alloc(ghtb);
    if(node == NULL){
        return -1;
    }

    node->used = 0;
    node->inwr = 0;
    node->reserved = 0;
    node->key_len  = key_len;
    node->key_hash = ghash_get_key(ghtb, key_len, key_dat);
    memcpy(node->key_dat, key_dat, key_len);

    node->data.udata = udata;

    entry = ghash_entry_get_by_id(ghtb, node->key_hash);

    ghash_entry_lock(entry);
    list_add_tail(&node->list, &entry->node_list);
    entry->node_cnt ++;
    ghash_entry_unlock(entry);

    return 0;
}

int ghash_node_del(void *t, int key_len, void *key_dat, long *pudata)
{
    ghash_table_t *ghtb = (ghash_table_t *)t;
    ghash_entry_t *entry;
    ghash_inner_node_t *node;
    ghash_inner_node_t *del_node = NULL;
    int            ret = -1;

    assert(t);
    assert(key_len);
    assert(key_dat);

    entry = ghash_entry_get(ghtb, key_len, key_dat);

    ghash_entry_lock(entry);

    node = ghash_node_find(entry, key_len, key_dat);
    if(node && node->used == 0){
        del_node = node;

        list_del(&del_node->list);
        entry->node_cnt--;
        ret = 0;
    }

    ghash_entry_unlock(entry);

    if(del_node){
        if(pudata){
            *pudata = del_node->data.udata;
        }

        ghash_node_free(ghtb, del_node);
    }

    return ret;
}

int ghash_node_request(void *t, int key_len, void *key_dat, ghash_node_t **pnode, int flag)
{
    ghash_table_t *ghtb = (ghash_table_t *)t;
    ghash_entry_t *entry;
    ghash_inner_node_t *node = NULL;
    int  ret = -1;

    assert(t);
    assert(key_len);
    assert(key_dat);
    assert(pnode);

    entry = ghash_entry_get(ghtb, key_len, key_dat);

    if(entry->node_magic != 34598701){
         printf("%p ghash_node_request %u fail! at %s:%d, %s\r\n",&entry->lock, *(((uint32_t *)(&entry->lock))+2),__FILE__, __LINE__,__func__);
    }
    ghash_entry_lock(entry);

    //to find node
    node = ghash_node_find(entry, key_len, key_dat);
    if(node && node->inwr == 0){
        if(flag & GHASH_OP_WR){
            if(node->used == 0){
                node->used++;
                node->inwr = 1;
                *pnode = &node->data;
                ret = 0;
            }
        }else{
            node->used++;
            *pnode = &node->data;
            ret = 0;
        }
    }

    ghash_entry_unlock(entry);

    return ret;
}

int ghash_node_release(void *t, ghash_node_t *pnode)
{
    ghash_table_t *ghtb = (ghash_table_t *)t;
    ghash_entry_t *entry;
    ghash_inner_node_t *node;

    assert(t);
    assert(pnode);

    node = container_of(pnode, ghash_inner_node_t, data);
    entry = ghash_entry_get_by_id(ghtb, node->key_hash);

    ghash_entry_lock(entry);

    assert(node->used);
    node->used--;
    node->inwr = 0;

    ghash_entry_unlock(entry);

    return 0;
}

int ghash_node_usrproc(void *t, int key_len, void *key_dat, ghash_proc_t proc, void *uarg)
{
    ghash_table_t       *ghtb = (ghash_table_t *)t;
    ghash_entry_t       *entry;
    ghash_inner_node_t  *node;
    int  ret = -2;

    assert(t);
    assert(key_len);
    assert(key_dat);
    assert(proc);

    entry = ghash_entry_get(ghtb, key_len, key_dat);

    ghash_entry_lock(entry);

    node = ghash_node_find(entry, key_len, key_dat);
    if(node && node->used == 0){
        ret = proc(uarg, t, &node->data);
    }

    ghash_entry_unlock(entry);

    return ret;
}

int ghash_node_foreach_del(void *t, ghash_proc_t clean, void *uarg)
{
    ghash_table_t       *ghtb = (ghash_table_t *)t;
    ghash_entry_t       *entry;
    int  entry_id;
    int  entry_max;
    int  ret = 0;

    assert(t);
    assert(clean);

    entry_max = ghtb->entry_mask + 1;
    for(entry_id = 0; entry_id < entry_max; entry_id++){
        struct list_head  clean_list;
        struct list_head  *iter, *next;

        INIT_LIST_HEAD(&clean_list);
        entry = ghash_entry_get_by_id(ghtb, entry_id);

        ghash_entry_lock(entry);
        list_for_each_safe(iter, next, &entry->node_list){
            ghash_inner_node_t  *cur_node;

            cur_node = list_entry(iter, ghash_inner_node_t, list);
            if(cur_node->used){
                continue;
            }

            if(0 == clean(uarg, t, &cur_node->data)){
                entry->node_cnt --;
                list_del(&cur_node->list);
                list_add(&cur_node->list, &clean_list);
            }
        }
        ghash_entry_unlock(entry);

        list_for_each_safe(iter, next, &clean_list){
            ghash_inner_node_t  *cur_node;

            cur_node = list_entry(iter, ghash_inner_node_t, list);
            //maybe not need
            list_del(&cur_node->list);
            ghash_node_free(ghtb, cur_node);
            ret++;
        }
    }

    return ret;
}

int  ghash_node_traverse(void *t, ghash_proc_t usr_proc, void *uarg)
{
    ghash_table_t       *ghtb = (ghash_table_t *)t;
    ghash_entry_t       *entry;
    int  entry_id;
    int  entry_max;


    assert(t);
    assert(usr_proc);

    entry_max = ghtb->entry_mask + 1;
    for(entry_id = 0; entry_id < entry_max; entry_id++){

        struct list_head  *iter, *next;

        entry = ghash_entry_get_by_id(ghtb, entry_id);

        ghash_entry_lock(entry);
        list_for_each_safe(iter, next, &entry->node_list){
            ghash_inner_node_t  *cur_node;

            cur_node = list_entry(iter, ghash_inner_node_t, list);

            if(cur_node != NULL){
                usr_proc(uarg, t, &cur_node->data);
            }
        }
        ghash_entry_unlock(entry);
    }

    return 0;
}


int ghash_hold_get(void *t, int key_len, void *key_dat, ghash_node_t **pnode)
{
    ghash_table_t *ghtb = (ghash_table_t *)t;
    ghash_entry_t *entry;
    ghash_inner_node_t *node = NULL;

    assert(t);
    assert(key_len);
    assert(key_dat);
    assert(pnode);

    entry = ghash_entry_get(ghtb, key_len, key_dat);
    
    ghash_entry_lock(entry);

    //to find node
    node = ghash_node_find(entry, key_len, key_dat);

    if(NULL == node) {
        ghash_entry_unlock(entry);
        return -1;
    }

    node->used++;
    *pnode = &node->data;

    return 0;
}

int ghash_put_get(void *t, ghash_node_t *pnode)
{
    ghash_table_t *ghtb = (ghash_table_t *)t;
    ghash_entry_t *entry;
    ghash_inner_node_t *node;

    assert(t);
    assert(pnode);

    node = container_of(pnode, ghash_inner_node_t, data);
    entry = ghash_entry_get_by_id(ghtb, node->key_hash);

    assert(node->used);
    node->used--;

    ghash_entry_unlock(entry);

    return 0;
}

