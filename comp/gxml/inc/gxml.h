#ifndef __GXML_H_
#define __GXML_H_
#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <string.h>
#define XML_FILE_NAME "media.xml"
#define DFT_MEDIAIP_NAME "dft-media-ip.xml"
#define DFT_MEDIAUSER_NAME "dft-media-user.xml"
#define XML_ACTIVE_HOST_NAME "dft-media-host.xml"

typedef void * gxmlhnd;
typedef void * gxmlattrhnd;

typedef int (*gxml_func)(void *arg, gxmlhnd n);
typedef struct _gxml_attr
{
    struct _gxml_attr *next;
    char *key;
    char *value;
    int kl;
    int vl;
}gxml_attr;
typedef struct _gxml_node
{
    char *name;
    char *content;
    int nl;
    int cl;
    gxml_attr *attr;
    struct _gxml_node *child;
    struct _gxml_node *parent;
    struct _gxml_node *sibling;
}gxml_node;

extern int gxml_init();
extern int gxml_deinit();
//for create xml tree from xml file
extern gxmlhnd gxml_build_tree(char *filename);
static inline gxmlhnd gxml_load(char *filename){
    return gxml_build_tree(filename);
}

//for save xml tree to xml file
extern int gxml_save(gxmlhnd hnd, const char *filename);

//for destroy a xml tree
extern void gxml_destroy_tree(gxmlhnd hnd);
static inline void  gxml_node_free(gxmlhnd hnd){
    gxml_destroy_tree(hnd);
}

extern int gxml_dellink(gxmlhnd hnd);
//delete from xml tree and destroy the node
static inline void gxml_node_delete(gxmlhnd node){
    if(0 != gxml_dellink(node))
        return;
    gxml_destroy_tree(node);
}

//for create xml root node 
extern gxmlhnd gxml_create_node(const char *key);
extern int gxml_add_content(gxmlhnd hnd, const char *content, int clen);
static inline gxmlhnd gxml_node_alloc(const char *label, const char *content){
    gxmlhnd n;
    n = gxml_create_node(label);
    if(!n)
        return NULL;
    if(content){
        if(-1 == gxml_add_content(n, content, strlen(content))){
            gxml_destroy_tree(n);
            return NULL;
        }
    }
    return n;        
}

//get label and get content from xml tree
extern char *gxml_getlable(gxmlhnd hnd);
extern char *gxml_get_content(gxmlhnd hnd);
extern gxmlhnd gxml_get_child(gxmlhnd hnd, char *key);
extern gxmlhnd gxml_next_sibling(gxmlhnd hnd, char *key);
extern gxmlhnd gxml_add_child(gxmlhnd hnd, const char *key, const char *content);
extern gxmlhnd gxml_getparent(gxmlhnd hnd);
extern int gxml_link2child(gxmlhnd hnd, gxmlhnd child);
extern int gxml_link2sibling(gxmlhnd hnd, gxmlhnd sibling);
static inline void *gxml_add_sibling(gxmlhnd node, const char *label, const char *content){
    gxmlhnd n = gxml_node_alloc(label, content);
    if(!n)
        return NULL;
    if(0 != gxml_link2sibling(node, n)){
        gxml_destroy_tree(n);
        return NULL;
    }
    return n;
}

//for all operation on xml attribute
extern int gxml_add_attr(gxmlhnd hnd, int npair, ...);
extern void gxml_del_attr(gxmlhnd hnd, char *k);
extern void gxml_destroy_xmlattr(gxmlhnd hnd);
extern gxmlattrhnd gxml_get_firstattr(gxmlhnd hnd);
extern gxmlattrhnd gxml_get_nextattr(gxmlattrhnd attr);
extern char *gxml_get_attrkey(gxmlattrhnd attr);
extern char *gxml_get_attrvalue(gxmlattrhnd attr);

//search in xml tree and do gxml_func
extern int gxml_foreach_specnode(char *, int, gxmlhnd, void *, gxml_func);



#ifdef __cplusplus
}
#endif

#endif/*__GXML_H_*/

