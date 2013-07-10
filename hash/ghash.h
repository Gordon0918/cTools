#ifndef _GHASH_INC_
#define _GHASH_INC_

#ifdef _cplusplus
extern "C" {
#endif

#define GHASH_KEY_SIZE_MAX  128

enum {
    GHASH_ALG_AUTO      = 0,
    GHASH_ALG_JS        = 1,
    GHASH_ALG_ELF       = 2,
};

enum {
    GHASH_OP_RD         = 0,
    GHASH_OP_WR         = 1,
};

typedef struct _ghash_node_t {
    long udata;
}ghash_node_t;

typedef struct _ghash_node_info_t {
    char *key_date;
    long  value_date;
}ghash_node_info_t;

typedef int (*ghash_proc_t)(void *uarg, void *t, ghash_node_t *node);

void *ghash_tbl_create(int node_max, int alg);
int   ghash_tbl_delete(void *t);

int ghash_node_add(void *t, int key_len, void *key_dat, long udata);
int ghash_node_del(void *t, int key_len, void *key_dat, long *pudata);

int ghash_node_request(void *t, int key_len, void *key_dat, ghash_node_t **pnode, int flag);
int ghash_node_release(void *t, ghash_node_t *node);
int ghash_node_usrproc(void *t, int key_len, void *key_dat, ghash_proc_t proc, void *uarg);
int ghash_node_foreach_del(void *t, ghash_proc_t judgment, void *uarg);
int ghash_node_traverse(void *t, ghash_proc_t usr_proc, void *uarg);
//int ghash_node_get_all_node(void *t, int *nums, ghash_node_info_t *nodes,int max_nodes);
//mickey add
int ghash_hold_get(void *t, int key_len, void *key_dat, ghash_node_t **pnode);
int ghash_put_get(void * t, ghash_node_t * pnode);


#ifdef _cplusplus
}
#endif

#endif /* _GHASH_INC_ */

