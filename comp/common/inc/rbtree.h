/* 
    rbtree.h :

    kevin.zheng@gmail.com
    2012/09/02
*/

#ifndef __M_RBTREE_H__
#define __M_RBTREE_H__



#define RB_RED      0
#define RB_BLACK    1

/*
    general function types

    cmp_key_t:  return value:   0   -- key1 == key2
                                1   -- key1 >  key2
                                -1  -- key2 <  key2
    
    get_key_t:  get key via structure stub

    get_rbcolor_t:  get color via red black tree stub
    set_rbcolor_t:  set color via red black tree stub. color is either RB_RED or RB_BLACK
*/

typedef int (* cmp_key_t)(void* key1, void* key2);
typedef void*    (* get_key_t)(void* stub);
typedef int      (* get_rbcolor_t)(void* stub);
typedef void     (* set_rbcolor_t)(void* stub, int color);


typedef struct st_bst_stub
{
    struct st_bst_stub* parent;
    struct st_bst_stub* left;
    struct st_bst_stub* right;
}m_bst_stub;


typedef void (*free_node_t)(m_bst_stub* bst_stub);

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// binary searching tree(left < node <= right)
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

m_bst_stub* bst_successor(m_bst_stub* x);
m_bst_stub* bst_predecessor(m_bst_stub* x);

m_bst_stub* bst_search(m_bst_stub* root, void* key, cmp_key_t cmp_key, get_key_t get_key);
m_bst_stub* bst_lesssearch(m_bst_stub* t, void* key, cmp_key_t cmp_key, get_key_t get_key);

void bst_insert(m_bst_stub** root, m_bst_stub* x, cmp_key_t cmp_key, get_key_t get_key);

//return: the node removed from binary searching tree. NULL means remove fail
m_bst_stub* bst_remove(m_bst_stub** root, void* key, cmp_key_t cmp_key, get_key_t get_key);
void bst_remove_node(m_bst_stub** root, m_bst_stub* x);

void bst_free_all(m_bst_stub** root, free_node_t free_node);

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// red black tree
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void rbt_insert(m_bst_stub** root, m_bst_stub* x, cmp_key_t cmp_key, get_key_t get_key, 
                          get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor);

m_bst_stub* rbt_remove(m_bst_stub** root, void* key, cmp_key_t cmp_key, get_key_t get_key, 
                                 get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor);
void rbt_remove_node(m_bst_stub** root, m_bst_stub* x, get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor);

m_bst_stub *rbt_find_max(m_bst_stub* t);
m_bst_stub *rbt_find_min(m_bst_stub* t);



#endif // __M_RBTREE_H__
