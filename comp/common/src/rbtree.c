/* 
    M_rbtree.c : 

    kevin.zheng@gmail.com
    2012/09/02
*/
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"



#define INLINE inline


////////////////////////////////////////////////////////////////////////
// binary searching tree
////////////////////////////////////////////////////////////////////////

m_bst_stub* bst_successor(m_bst_stub* x)
{
    if(x->right)
    {
        x = x->right;
        while(x->left)
            x = x->left;
        return x;
    }
    else
    {
        while(x->parent && x != x->parent->left)
            x = x->parent;
        return x->parent;
    }
}
m_bst_stub* bst_predecessor(m_bst_stub* x)
{
    if(x->left)
    {
        x = x->left;
        while(x->right)
            x = x->right;
        return x;
    }
    else
    {
        while(x->parent && x!= x->parent->right)
            x = x->parent;
        return x->parent;
    }
}

static INLINE void bst_left_rotate(m_bst_stub** root, m_bst_stub* x)
{
    m_bst_stub* y = NULL;

    if(!x->right)
        return;

    y = x->right;
    if(x->parent)
    {
        if(x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    else
        *root = y;

    y->parent = x->parent;
    x->parent = y;
    x->right = y->left;
    y->left = x;
    if(x->right)
        x->right->parent = x;
}

static INLINE void bst_right_rotate(m_bst_stub** root, m_bst_stub* x)
{
    m_bst_stub* y = NULL;

    if(!x->left)
        return;

    y = x->left;
    if(x->parent)
    {
        if(x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    else
        *root = y;

    y->parent = x->parent;
    x->parent = y;
    x->left = y->right;
    y->right = x;
    if(x->left)
        x->left->parent = x;
}


m_bst_stub* bst_search(m_bst_stub* t, void* key, cmp_key_t cmp_key, get_key_t get_key)
{
    int cmp = 0;
    while(t)
    {
        cmp = cmp_key(key, get_key(t));
        if(!cmp)
            return t;
        else if(cmp < 0)
            t = t->left;
        else
            t = t->right;
    }
    return NULL;
}

/*mickey add*/
m_bst_stub* bst_lesssearch(m_bst_stub* t, void* key, cmp_key_t cmp_key, get_key_t get_key)
{
    int cmp = 0;
    m_bst_stub *s = t;
    
    if(NULL == t){
        return NULL;
    }
    
    while(t)
    {
        cmp = cmp_key(key, get_key(t));
        s = t;
        if(!cmp)
            return t;            
        else if(cmp < 0)
            t = t->left;
        else
            t = t->right;
    }

    cmp = cmp_key(key, get_key(s));
    if(cmp<0){
        return bst_predecessor(s);    
    }else{
        return s;
    }    
}


void bst_insert(m_bst_stub** root, m_bst_stub* x, cmp_key_t cmp_key, get_key_t get_key)
{
    m_bst_stub* p = NULL;
    m_bst_stub* t = *root;

    while(t)
    {
        p = t;
        t = (cmp_key(get_key(x), get_key(t))) < 0 ? t->left : t->right;
    }
    if(!p)
        *root = x;
    else
    {
        if((cmp_key(get_key(x), get_key(p))) < 0) //x->key < p->key
            p->left = x;
        else
            p->right = x;
    }
    x->parent = p;
    x->left = x->right = NULL;
}

void bst_remove_node(m_bst_stub** root, m_bst_stub* x)
{
    m_bst_stub *y, *c;

    //y is the node which will be removed
    y = (!x->left || !x->right) ? x : bst_successor(x);
    //y has at most 1 child, c is that child. c also can be NULL
    c = y->left ? y->left : y->right;

    //splicing y out
    if(c)
        c->parent = y->parent;
    if(y->parent)
    {
        if(y == y->parent->left)
            y->parent->left = c;
        else
            y->parent->right = c;
    }

    //replace x with y. x still remain its position, but lost
    //pointers other points it
    if(x != y)//if y is x's successor, y will replace the position of x
    {
        if(x->left)
            x->left->parent = y;
        if(x->right)
            x->right->parent = y;

        if(x->parent)
        {
            if(x->parent->left == x)
                x->parent->left = y;
            else
                x->parent->right = y;
        }
        else
            *root = y;

        y->parent = x->parent;
        y->left = x->left;
        y->right = x->right;
    }
    else
    {
        if(!x->parent)
            *root = c;
    }
}

m_bst_stub* bst_remove(m_bst_stub** root, void* key, cmp_key_t cmp_key, get_key_t get_key)
{
    m_bst_stub* x = bst_search(*root, key, cmp_key, get_key);
    if(x)
        bst_remove_node(root, x);
    return x;
}

void bst_free_all(m_bst_stub** root, free_node_t free_node)
{
    m_bst_stub* head = *root;
    m_bst_stub* last = head;
    m_bst_stub* tmp;
    while(head)
    {
        if(head->left)
        {
            last->parent = head->left;
            last = last->parent;
        }
        if(head->right)
        {
            last->parent = head->right;
            last = last->parent;
        }
        
        last->parent = NULL;
        
        tmp = head->parent;
        free_node(head);
        head = tmp;
    }

    *root = NULL;
}

////////////////////////////////////////////////////////////////////////
// red black tree
////////////////////////////////////////////////////////////////////////

static INLINE int _rbt_is_red(m_bst_stub* x, get_rbcolor_t get_rbcolor)
{
    if(x && (get_rbcolor(x) == RB_RED))
        return 1;
    else
        return 0;
}

static INLINE void _rbt_set_color(m_bst_stub* x, int color, set_rbcolor_t set_rbcolor)
{
    if(x)
        set_rbcolor(x, color);
}

void rbt_insert(m_bst_stub** root, m_bst_stub* x, cmp_key_t cmp_key, get_key_t get_key, 
                get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor)
{
    m_bst_stub *y, *z;

    bst_insert((m_bst_stub**)root, (m_bst_stub*)x, cmp_key, get_key);
    set_rbcolor(x, RB_RED);
    y = x->parent;

    while(y && get_rbcolor(y) == RB_RED)
    {
        z = y->parent;
        if(_rbt_is_red(z->left, get_rbcolor) && _rbt_is_red(z->right, get_rbcolor))
        {
            _rbt_set_color(z->left, RB_BLACK, set_rbcolor);
            _rbt_set_color(z->right, RB_BLACK, set_rbcolor);
            set_rbcolor(z, RB_RED);
            x = z;
            y = z->parent;
        }
        else if(y == z->left)//z must be black here
        {
            if(y->right == x)
            {
                bst_left_rotate((m_bst_stub**)root, (m_bst_stub*)y);
                x = y;
                y = x->parent;
            }
            bst_right_rotate((m_bst_stub**)root, (m_bst_stub*)z);
            set_rbcolor(y, RB_BLACK);
            set_rbcolor(z, RB_RED);
            return;
        }
        else
        {
            if(y->left == x)
            {
                bst_right_rotate((m_bst_stub**)root, (m_bst_stub*)y);
                x = y;
                y = x->parent;
            }
            bst_left_rotate((m_bst_stub**)root, (m_bst_stub*)z);
            set_rbcolor(y, RB_BLACK);
            set_rbcolor(z, RB_RED);
            return;
        }
    }
    set_rbcolor(*root, RB_BLACK);
}

void rbt_remove_node(m_bst_stub** root, m_bst_stub* x, get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor)
{
    m_bst_stub *y, *c, *pc, *w;
    int    pcolor;

    //y is the node which will be removed.
    y = (!x->left || !x->right) ? x : (m_bst_stub*)bst_successor((m_bst_stub*)x);
    //y has at most 1 child, c is that child. c also can be NULL
    c = y->left ? y->left : y->right;

    pc = y->parent;
    pcolor = get_rbcolor(y);

    //splicing y out
    if(c)
        c->parent = pc;
    if(pc)
    {
        if(y == pc->left)
            pc->left = c;
        else
            pc->right = c;
    }

    if(!x->parent)
        *root = y;

    if(x == y && !pc)
        *root = c;

    //add 2012/8/18
    if(!*root || pcolor == RB_RED)
        goto out;
    if(c && (get_rbcolor(c) == RB_RED))
    {
        set_rbcolor(c, RB_BLACK);
        goto out;
    }

    //now x is black and y is black(can be NULL)
    while(c != *root && pc && !_rbt_is_red(c, get_rbcolor)) //add pc condition, 2012/8/18
    {
        if(c == pc->left)
        {
            w = pc->right;
            if(_rbt_is_red(w, get_rbcolor))
            {
                //w can't be NULL
                set_rbcolor(w, RB_BLACK);
                set_rbcolor(pc, RB_RED);
                bst_left_rotate((m_bst_stub**)root, (m_bst_stub*)pc);
                w = pc->right;
            }
            if(!_rbt_is_red(w->left, get_rbcolor) && !_rbt_is_red(w->right, get_rbcolor))
            {
                _rbt_set_color(w, RB_RED, set_rbcolor);
                c = pc;
                pc = pc->parent;
                //continue;
            }
            else 
            {
                if(!_rbt_is_red(w->right, get_rbcolor))
                {
                    _rbt_set_color(w->left, RB_BLACK, set_rbcolor);
                    set_rbcolor(w, RB_RED);
                    bst_right_rotate((m_bst_stub**)root, (m_bst_stub*)w);
                    w = pc->right;
                }
                else
                {
                    _rbt_set_color(w, get_rbcolor(pc), set_rbcolor);
                    set_rbcolor(pc, RB_BLACK);
                    _rbt_set_color(w->right, RB_BLACK, set_rbcolor);
                    bst_left_rotate((m_bst_stub**)root, (m_bst_stub*)pc);
                    c = *root;
                }
            }
        }
        else
        {
            w = pc->left;
            if(_rbt_is_red(w, get_rbcolor))
            {
                //w can't be NULL
                set_rbcolor(w, RB_BLACK);
                set_rbcolor(pc, RB_RED);
                bst_right_rotate((m_bst_stub**)root, (m_bst_stub*)pc);
                w = pc->left;
            }
            if(!_rbt_is_red(w->left, get_rbcolor) && !_rbt_is_red(w->right, get_rbcolor))
            {
                _rbt_set_color(w, RB_RED, set_rbcolor);
                c = pc;
                pc = pc->parent;
                //continue;
            }
            else 
            {
                if(!_rbt_is_red(w->left, get_rbcolor))
                {
                    _rbt_set_color(w->right, RB_BLACK, set_rbcolor);
                    set_rbcolor(w, RB_RED);
                    bst_left_rotate((m_bst_stub**)root, (m_bst_stub*)w);
                    w = pc->left;
                }
                else
                {
                    _rbt_set_color(w, get_rbcolor(pc), set_rbcolor);
                    set_rbcolor(pc, RB_BLACK);
                    _rbt_set_color(w->left, RB_BLACK, set_rbcolor);
                    bst_right_rotate((m_bst_stub**)root, (m_bst_stub*)pc);
                    c = *root;
                }
            }
        }
    }
    _rbt_set_color(c, RB_BLACK, set_rbcolor);

out:
    if(x != y)//if y is x's successor, y will replace the position of x
    {
        if(x->left)
            x->left->parent = y;
        if(x->right)
            x->right->parent = y;

        if(x->parent)
        {
            if(x->parent->left == x)
                x->parent->left = y;
            else
                x->parent->right = y;
        }

        y->parent = x->parent;
        y->left = x->left;
        y->right = x->right;
        set_rbcolor(y, get_rbcolor(x));
    }
}

m_bst_stub* rbt_remove(m_bst_stub** root, void* key, cmp_key_t cmp_key, get_key_t get_key, 
                       get_rbcolor_t get_rbcolor, set_rbcolor_t set_rbcolor)
{
    m_bst_stub* x = (m_bst_stub*)bst_search((m_bst_stub*)(*root), key, cmp_key, get_key);
    
    if(x)
        rbt_remove_node(root, x, get_rbcolor, set_rbcolor);
    return x;
}


m_bst_stub *rbt_find_max(m_bst_stub* t)
{
    if(NULL == t) return NULL;
    
    while(t)
    {
        if(!t->right){
            return t;
        }
        t = t->right;        
    }
    return NULL;
}

m_bst_stub *rbt_find_min(m_bst_stub* t)
{
    if(NULL == t) 
        return NULL;
    
    while(t)
    {
        if(!t->left){
            return t;
        }
        t = t->left;        
    }
    return NULL; 
}


