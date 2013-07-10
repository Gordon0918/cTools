#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "gxml.h"
#include "gutils.h"

#define VISIABLE_CHAR_START 32
#define VISIABLE_CHAR_END    126


typedef struct _q_xml_buf{
    char *data;
    int data_len;
    gxml_node *xml;
    struct _q_xml_buf *next;
}q_xml_buf;


typedef struct _s_xml_node
{
    struct _s_xml_node *next;
    gxml_node *xml;
}s_xml_node;


static int xmlno = 0;
static int total_mem = 0;

static inline void *xmlmalloc(size_t size){
    total_mem += size;
    return malloc(size);
}

static inline void *_xmlmalloc(size_t size){
    long *p;

    p = (long *)malloc(size + sizeof(long));

    if(!p) return NULL;

    *p = size + sizeof(long);

    p++;   
    
    total_mem += size;

    return (void *)p;    
}

static inline void _xmlfree(void *p){
    long *h = (long *)p;

    h--;
    total_mem -= *h;
    free(h);    
}

static inline void xmlfree(void *p, int len){
    total_mem -= len;
    free(p);
}

int gxml_init()
{
    xmlno = 0;
    return 0;
}
int gxml_deinit()
{
    return 0;
}

static int gopenfile(char *filename)
{
    int fd = 0;
    fd = open(filename, O_RDONLY);
    if(-1 == fd)
    {
        return -1;
    }
    return fd;
}



/*three state
1.<?
2.<!
3.</
4.normal
*/

int findchar(char c, char *buf, int buflen)
{
    int i;
    
    for(i=0; i<buflen; i++)
    {
        if(c == buf[i])
        {
            return i;
        }
    }

    return -1;
}


static int findstr(char *needle, int nlen, char *buf, int buflen)
{
    int nc,j;
 
    for(nc=0; nc<buflen-nlen+1; nc++)
    {
        for(j=0;j<nlen;j++)
        {
            if(buf[nc+j] != needle[j])
            {
                break;
            }
        }
        if(j == nlen)
        {
            return nc;
        }
    }
    
    return -1;
    
}

static gxml_node *new_gxml()
{
    gxml_node *xml = (gxml_node *)xmlmalloc(sizeof(gxml_node));
    
    if(NULL == xml)
        return NULL;
        
    memset(xml,0,sizeof(gxml_node));    
    
    return xml;
}

static void free_gxml(gxml_node *xml)
{
    if(NULL == xml)
    {
        return ;
    }
    xmlfree(xml, sizeof(gxml_node));
}

static void free_gxml_node_attr(gxml_attr *xmlattr){
    gxml_attr *attr = NULL;
    gxml_attr *attrnext = NULL;

    if(!xmlattr) return ;
   
    if(NULL != xmlattr)
    {
        attr = xmlattr;
        attrnext = attr;
        while(attr)
        {
            attrnext = attr->next;
            if(attr->key)
                xmlfree(attr->key,attr->kl);
            if(attr->value)    
                xmlfree(attr->value,attr->vl);
            xmlfree(attr,sizeof(gxml_attr));
            attr = attrnext;
        }
    }
}

static void free_gxml_node(gxml_node *xml)
{
    if(!xml) return ;
    
    free_gxml_node_attr(xml->attr);

    xml->attr = NULL;
    
    if(NULL != xml->content)
    {
        xmlfree(xml->content,xml->cl);        
        xml->content = NULL;
    }
    if(NULL != xml->name)
    {
        xmlfree(xml->name,xml->nl);
        xml->name = NULL;
    }
    xml->parent = NULL;
    free_gxml(xml); 
}


static gxml_node *make_root()
{
    char buf[64];
    int len = 0;
    
    gxml_node *xml = new_gxml();

    if(NULL == xml )
    {
        return NULL;
    }

    len = sprintf(buf, "gxml%d",xmlno);
    if(len < 0)
    {
        xmlfree(xml, sizeof(gxml_node));
        return NULL;
    }

    xml->name = xmlmalloc(galign(len,8));

    if(NULL == xml->name)
    {
        xmlfree(xml,sizeof(gxml_node));
        return NULL;
    }
    xml->nl = galign(len,8);
    memcpy(xml->name, buf, len);

    xml->name[len] = '\0';

    xmlno ++;
    
    return xml;
    
}

static int create_xml(const char *key, int key_len, gxml_node **xml )
{
    gxml_node *p;
        /*create child key*/
    p = new_gxml();
    if(NULL == p)
    {
        return -1;
    }

    p->name = xmlmalloc(galign(key_len, 8));
    if(NULL == p->name)
    {
        free_gxml(p);
        return -1;
    }
    p->nl = galign(key_len, 8);
    memcpy(p->name, key, key_len);
    p->name[key_len] = '\0';

    *xml = p;
    
    return 0;
}

static int get_first_key(char *data, int nlen, char **key, int *key_len, int *pos)
{
    int right;
    char *p;
    int len;
    
    if(-1 == (right = findchar('>', data+1, nlen-1)))
    {
        return -1;
    }

    p = data+1;
    len = right;
    *pos = right;   /*> pos*/
    if(0 < (right = findchar(' ',p,len)))
    {
        len = right;
    }

    *key = p;
    *key_len = len;
    
    return 0;    
}

static int drop_unconcern_octs(char *buf, int buf_len)
{
    int nloop = 0,nc = buf_len,right;
    
    while(nloop<nc)
    {
        if(buf[nloop] < VISIABLE_CHAR_START || buf[nloop] >VISIABLE_CHAR_END)
        {
            nloop ++;
            continue;
        }
        if('<' == buf[nloop] )
        {
            if((nloop + 1) >= nc)
            {
                return -1;
            }
            if(-1 == (right = findchar('>', &buf[nloop+1], nc-(nloop+1))))
            {
                return -1;
            }
            
            if('!' == buf[nloop + 1] || '?' == buf[nloop + 1])
            {
               nloop = nloop + 2 + right;   //jmp to '>' next byte
               continue;
            }
            else
            {
                return nloop;
            }
        }else if(' ' == buf[nloop]){
            nloop ++;
        }else{
            return nloop;
        }        
     }
     return -1;
}

static void add_to_attrlist(gxml_attr *new_attr, gxml_node *n)
{
    gxml_attr *attr;
    
    if(NULL == n->attr)
    {
        n->attr = new_attr;
    }else{
        attr = n->attr;
        while(attr)
        {
            if(NULL == attr->next)
            {
                break;
            }
            attr = attr->next;
        }
        attr->next = new_attr;
    }
}

static void del_from_attrlist(gxml_attr *attr, gxml_node *n){
    gxml_attr *cur, *prev = NULL;

    cur = n->attr;

    while(cur){
        if(attr == cur) break;
        prev = cur;
        cur = cur->next;
    }
    if(!cur)
        return;
    if(!prev)
        n->attr = cur->next;
    else
        prev->next = cur->next;
    cur->next = NULL;
}

gxml_attr *gxml_create_attr(){
    gxml_attr *xmlattr = xmlmalloc(sizeof(gxml_attr));

    if(NULL == xmlattr)
    {
        return NULL;
    }
    
    xmlattr->value = NULL;
    xmlattr->next = NULL;
    return xmlattr;
}


static int create_value(char *buf, int nc, gxml_attr *attr)
{
    attr->value = xmlmalloc(galign(nc, 8));
    if(NULL ==attr->value)
    {
        return -1;
    }
    attr->vl = galign(nc, 8);
    memcpy(attr->value, buf, nc);
    attr->value[nc] = '\0';
    return 0;
}

static int create_key(char *buf, int nc, gxml_attr *attr)
{
    attr->key = xmlmalloc(galign(nc, 8));
    if(NULL ==attr->key)
    {
        return -1;
    }
    attr->kl = galign(nc,8);
    memcpy(attr->key, buf, nc);
    attr->key[nc] = '\0';
    return 0;
}

static int do_parser_attr(char *buf, int nc,gxml_node *n)
{
    int drop,right,nlen = nc;
    char *data = buf;
    gxml_attr *xmlattr = NULL;

    while(1)
    {
        drop = drop_unconcern_octs(data, nlen);
        if(-1 == drop)
        {
            return -1;
        }
        data += drop;
        nlen -= drop;
        if(data[0] == '>')
        {
            return 0;
        }
        if(-1 == (right = findchar('=', data, nlen)))
        {
            return -1;
        }
        if(NULL == (xmlattr = gxml_create_attr())){
            return -1;
        }
        
        add_to_attrlist(xmlattr, n);
        
        if(0 != create_key(data, right, xmlattr)){
            return -1;
        }
        
        data += right + 1;
        nlen -= right + 1;
        if(nlen < 0)
        {
            return -1;
        }
        
        if(-1 == (right = findchar(' ', data, nlen)))
        {
           if(-1 == create_value(data, nlen, xmlattr))
            {
                return -1;
            }
           return 0;
        }else{
            if(-1 == create_value(data, right, xmlattr))
            {
                return -1;
            }
        }
        data += right;
        nlen -= right;
        if(nlen < 0)
        {
            return -1;
        }
            
    }
    return -1;
}

#if 0
static int gxml_strcpy(char *d, int dlen, char *src, char slen)
{
    if(dlen <= slen)
    {
        return -1;
    }

    memcpy(d,src,slen);
    *(d + slen) = '\0';
    return 0;
}
#endif
static int gxml_push(q_xml_buf **xml_q,char *data, int dlen, gxml_node *xml)
{
    q_xml_buf *buf = NULL;

    if(NULL == data || NULL == xml)
    {
        return -1;
    }

    buf = xmlmalloc(sizeof(q_xml_buf));
    if(NULL == buf)
    {
        return -1;
    }

    buf->data = data;
    buf->data_len = dlen;
    buf->xml = xml;
    buf->next = NULL;

    if(NULL == *xml_q)
    {
        *xml_q = buf;

    }else{
        buf->next = *xml_q;
        *xml_q = buf;
    }

    return 0;
}

static int gxml_pop(q_xml_buf **xml_q, char **data, int *dlen, gxml_node **xml)
{
    q_xml_buf *buf;
    
    assert(data&&dlen&&xml);
    if(NULL == *xml_q)
    {
        return -1;
    }

    buf = *xml_q;
    *xml_q = buf->next;

    *data = buf->data;
    *dlen = buf->data_len;
    *xml = buf->xml;

    xmlfree(buf, sizeof(q_xml_buf));

    return 0;
}

static void gxml_destroy_stack(q_xml_buf **xml_buf)
{
    char *data;
    int len;
    gxml_node *xml;
    while(0 == gxml_pop(xml_buf,&data,&len,&xml));
}

static int sxml_push(s_xml_node **h,gxml_node *xml){
    s_xml_node *buf;

    assert(xml);

    buf = xmlmalloc(sizeof(s_xml_node));
    if(NULL == buf){
        return -1;
    }
    
    buf->xml = xml;
    buf->next = NULL;

    if(NULL == *h){
        *h = buf;
    }else{
        buf->next = *h;
        *h = buf;
    }
    return 0;
}

static int sxml_pop(s_xml_node **h,gxml_node **xml)
{
    s_xml_node *buf;

    if(NULL == *h)
    {
        return -1;
    }

    buf = *h;
    *h = (*h)->next;

    *xml = buf->xml;

    xmlfree(buf, sizeof(s_xml_node));
    
    return 0;    
}

static void sxml_destroy(s_xml_node **s)
{
    gxml_node *xml;
    
    while(0 == sxml_pop(s,&xml));
    
}

static int do_parser_content(q_xml_buf **xml_buf, char *buf, int nc,gxml_node *n)
{
    char *data = buf;
    int nlen = nc,drop;

    drop = drop_unconcern_octs(data, nlen);
    if(-1==drop)
    {
        return -1;
    }
    data += drop;
    nlen -= drop;

    if('<' == data[0])
    {
        return gxml_push(xml_buf,data, nlen, n);
    }

    //gxml_strcpy(b, sizeof(b), data, nlen);

    //printf("==>[%d]==>%s\r\n",nlen, b);
    return gxml_add_content(n, data, nlen);    
}

int do_build_childxmltree(q_xml_buf **xml_buf, char *buf, int buflen, gxml_node *root)
{
    char *data = buf;
    int drop,datalen = buflen;
    gxml_node *xml = NULL,*xmlsibling = NULL;
    char *key=NULL;
    int key_len = 0,pos = 0;
    char b[1024];
    

    /*don't care statement, drop it*/
    drop = drop_unconcern_octs(data, datalen);
    if(-1 == drop){  
        return -1;
    }

    data += drop;
    datalen -= drop;

    while(datalen > 0){
        if(-1 == get_first_key(data,datalen,&key,&key_len,&pos)){
            return -1;
        }
        if(-1==create_xml(key,key_len,&xml)){
            return -1;
        }
        xml->parent = root;
        if(NULL == xmlsibling){
            root->child = xml;
            xmlsibling = xml;
        }else{
            xmlsibling->sibling = xml;
            xmlsibling = xml;
        }        
        /*add attribute*/
        if(' ' == *(key+key_len)){
            if(-1 == do_parser_attr(data+key_len+1, pos-key_len, xml)){
                return -1;
            }
        }
        data += pos+2;   /*two byte first is < last is >*/
        datalen -= pos+2;

        b[0] = '<';
        b[1] = '/';
        memcpy(&b[2], key, key_len);
        b[key_len + 2] = '>';
        b[key_len + 3] = '\0';

        pos = findstr(b, key_len+3, data, datalen);//find </key>
        if(-1 == pos )
        {  
            return -1;      
        }
        if(-1 == do_parser_content(xml_buf,data, pos, xml))
        {
            return -1;
        }

        data += pos + key_len;
        datalen -= pos + key_len;
        
        if(-1 == (pos = findchar('<', data, datalen)))
        {
            return 0;                
        }
        data += pos;
        datalen -= pos;
    }
    
    return 0;
}

static int do_build_xmltree(q_xml_buf **xml_buf)
{
    char *data;
    int d_len;
    gxml_node *root;

    while(0 == gxml_pop(xml_buf, &data, &d_len, &root))
    {
        if(-1 == do_build_childxmltree(xml_buf, data, d_len, root))
        {
            gxml_destroy_stack(xml_buf);
            return -1;
        }
    }
    return 0;
}

static int gxml_parser(char *buf, int nc, gxml_node **root)
{
    gxml_node *xml;
    q_xml_buf *xml_buf = NULL;

    xml = make_root();
    if(NULL == xml)
    {
        return -1;
    }
    *root = xml;

    if(-1 == gxml_push(&xml_buf,buf, nc, *root))
    {
        free_gxml_node(*root);
        return -1;
    }
    if(-1 == do_build_xmltree(&xml_buf))
    {
        gxml_destroy_tree(*root);
        return -1;
    }

    return 0;
}

static int gxml_readfile(int fd,char *fbuf,int fsize)
{
    char buf[1024];
    int nc,fbuflen = 0;
    
    while(1)
    {
        nc = read(fd,buf,sizeof(buf));
        if(-1 == nc )
        {
            switch(errno)
            {
                case EAGAIN:
                case EINTR:
                    usleep(1);
                    break;
                default:
                    return -1;
            }
        }else if(0 == nc){
            break;
        }else{
            memcpy(fbuf+fbuflen,buf,nc);
            fbuflen += nc;
        }
    }
    return 0;
}

gxmlhnd gxml_build_tree(char *filename)
{
    int fd = 0;
    struct stat gfstat;
    char *fbuf;
    gxml_node *root = NULL;

    if(NULL == filename)
    {
        return NULL;
    }

    if(-1 == (fd = gopenfile(filename)))
    {
        return NULL;
    }

    if(-1 == fstat(fd,&gfstat))
    {
        close(fd);
        return NULL;
    }

    if(NULL == (fbuf = xmlmalloc(galign(gfstat.st_size+8,8))))
    {
        close(fd);
        return NULL;
    }

    if(-1== gxml_readfile(fd,fbuf,gfstat.st_size))
    {
        xmlfree(fbuf,galign(gfstat.st_size+8,8));
        close(fd);
        return NULL;
    }

    memset(fbuf+gfstat.st_size,0,8);
    if(-1 == gxml_parser(fbuf, gfstat.st_size+8, &root))
    {
        xmlfree(fbuf,galign(gfstat.st_size+8,8));
        close(fd);      
        return NULL;
    }   
    xmlfree(fbuf,galign(gfstat.st_size+8,8));
    close(fd);

    return (gxmlhnd)root;
}

static int gxml_foreach_sibling(s_xml_node **s,
                         char *key, int len, 
                         gxml_node *root, 
                         void *arg, gxml_func f){
    gxml_node *sibling = NULL;
    
    if(NULL == key || NULL == root || NULL == f){
        return -1;
    }

    sibling = root;

    for(;NULL!= sibling;sibling = sibling->sibling){
        if(sibling->child){
            sxml_push(s,sibling->child);
        }
        if(len != strlen(sibling->name)){
            continue;
        }
        if(0 == memcmp(key,sibling->name,len)){
            if(-1 == f(arg,sibling)){
                return -1;
            }
        }
    }

    return 0;   
}

int gxml_foreach_specnode(char *key, int len, 
                          gxmlhnd hnd, 
                          void *arg, gxml_func f)
{
    gxml_node *xml = NULL;
    s_xml_node *s = NULL;
    gxml_node *root = (gxml_node *)hnd;

    

    if(NULL == root) return -1;
    
    sxml_push(&s,root);

    while(0 == sxml_pop(&s,&xml)){
        if(-1 == gxml_foreach_sibling(&s,key,len,xml,arg,f)){
            sxml_destroy(&s);
            return -1;
        }
    }
    return 0;
    
}


void gxml_destroy_tree(gxmlhnd hnd)
{
    gxml_node *top = (gxml_node *)hnd;
    gxml_node *head = (gxml_node *)hnd;
    gxml_node *n = NULL;

    while(head)
    {
        if(head->child)
        {
            top->parent = head->child;
            top = top->parent;
        }
        if(head->sibling)
        {
            top->parent = head->sibling;
            top = top->parent;
        }        
        top->parent = NULL;

        n = head;
        head = head->parent;
        free_gxml_node(n);
    }
}




static int gxml_getprevtag(int deep, char *buf, int len, int *taglen, gxml_node *xml){
    int tmplen = 0;
    int i;
    gxml_attr *attr;
    if(!buf || !len || !taglen)
        return -1;
        
    for(i=0; i< (deep<<2); i++){
        buf[i] = ' ';
        tmplen ++;    
    }

    tmplen += snprintf(buf+tmplen, len-tmplen,"<%s",xml->name);

    attr = xml->attr;
    while(attr){
        tmplen += snprintf(buf+tmplen, len-tmplen, " %s=%s",attr->key, attr->value);

        attr = attr->next;
    }
    tmplen+= snprintf(buf+tmplen, len-tmplen, ">");    

    *taglen = tmplen;

    
    return 0;
}

static int gxml_getcontent(char *buf, int len, int *ctlen, gxml_node *xml){
    int tmplen = 0;

    if(!buf || !len || !ctlen)
        return -1;
    
    tmplen += snprintf(buf+tmplen, len-tmplen, "%s", xml->content);

    *ctlen = tmplen;

    return 0;
}

static int gxml_getsuffix(int flag, int deep, char *buf, int len, int *suflen, gxml_node *xml){
    int tmplen = 0;
    int i;

    if(!buf || !len || !suflen)
        return -1;

    if(!flag){
        for(i=0; i< (deep<<2); i++){
            buf[i] = ' ';
            tmplen ++;    
        }
    }

    tmplen += snprintf(buf+tmplen, len-tmplen, "</%s>", xml->name);

    *suflen = tmplen;

    return 0;
    
}

static int gxml_write2file(int fd, gxml_node *root){
    q_xml_buf *sxml = NULL;
    char buf[1024] = {0};
    int len = 0;
    int deep = 0;
    int flag = 0;
    char *data = (char *)&deep;
    gxml_node *xml = NULL;

    xml = root;

    if(write(fd, "<?xml version=\"1.0\"?>\r\n", sizeof("<?xml version=\"1.0\"?>\r\n")-1)<0)
        return -1;
    
    while(xml){
        if(-1 == gxml_getprevtag(deep,buf, sizeof(buf), &len,xml)){
            goto errsave;
        }
        len = write(fd, buf, len);
        gxml_push(&sxml, data, deep, xml);
        if(xml->content){
            flag = 1;
            if(-1 == gxml_getcontent(buf, sizeof(buf), &len, xml)){
                goto errsave;
            }
            len = write(fd,buf,len);
        }
        xml = xml->child;
        if(xml){
            write(fd,"\r\n",2);
        }            
        deep ++;
        while(!xml){
            if(-1 == gxml_pop(&sxml, &data, &deep, &xml)){
                return 0;
            }
            if(-1 == gxml_getsuffix(flag,deep,buf, sizeof(buf), &len, xml)){
                goto errsave;
            }
            len = write(fd, buf, len);
            flag = 0;
            write(fd,"\r\n",2);
            xml=xml->sibling;
        }
    }
errsave:
    gxml_destroy_stack(&sxml);
    return -1;    
}

int gxml_save(gxmlhnd hnd, const char *filename){
    int fd;
//    int len;
    gxml_node *root = (gxml_node *)hnd;

    if(!root){
        return -1;
    }

    if(!filename){
        filename = root->name;
        if(!filename)
            return -1;
    }

        
    fd = open(filename, O_WRONLY|O_TRUNC|O_CREAT);
    if(fd < 0 ){
        return -1;
    }

    if(-1 == gxml_write2file(fd, root->child)){
        printf("write 2 file error\r\n");
    }
    
    close(fd); 

    return 0;
}

static gxml_node *gxml_getsibling_node(gxml_node *n, char *key){
    int len = strlen(key);
    while(n){
        if(strlen(n->name) == len){
            if(0 == memcmp(n->name, key, len))
                return n;
        }
        n = n->sibling;
    }
    return NULL;  
}

gxmlhnd gxml_get_child(gxmlhnd hnd, char *key){
    gxml_node *xml = (gxml_node *)hnd;
    
    xml = xml?(xml->child):NULL;
    if(NULL == key)
        return xml;

    return (gxmlhnd)gxml_getsibling_node(xml,key);
          
}

gxmlhnd gxml_next_sibling(gxmlhnd hnd, char *key){
    gxml_node *xml = (gxml_node *)hnd;
//    int len;

    xml = xml?(xml->sibling):NULL;

    if(NULL == key){
        return (gxmlhnd)xml;
    }
    return (gxmlhnd)gxml_getsibling_node(xml,key);    
}

char *gxml_get_content(gxmlhnd hnd){
    return hnd?((gxml_node *)hnd)->content:NULL;
}

gxmlhnd gxml_create_node(const char *key){
    gxml_node *xml = NULL;
    if(!key)
        return NULL;
    if( 0 != create_xml(key, strlen(key), &xml)){
        return NULL;
    }
    return (gxmlhnd)xml;
}

int gxml_add_content(gxmlhnd hnd, const char *content, int clen){
    gxml_node *n = (gxml_node *)hnd;
    
    if(!hnd || !content) return -1;

    if(n->content){
        xmlfree(n->content, n->cl);
        n->content = NULL;
        n->cl = 0;
    }

    n->content = xmlmalloc(galign(clen, 8));
    if(NULL == n->content)
    {
        return -1;
    }
    n->cl = galign(clen, 8);
    memcpy(n->content, content, clen);
    n->content[clen] = '\0';

    return 0;    
}

static gxml_attr *gxml_get_attr(gxml_node *n, char *key){
    gxml_attr *attr;
    if(!n || !key) return NULL;

    attr = n->attr;

    while(attr){
        if(0 == strcmp(attr->key,key))
            return attr;
        attr = attr->next;
    }
    return NULL;
}

int gxml_add_attr(gxmlhnd hnd, int npair, ...){
    long *p = (long *)&npair;
    char *k,*v;
    int i;
    gxml_attr *attr = NULL;
    gxml_node *n = (gxml_node *)hnd;

    if(!n || npair < 0)
        return -1;
    p++;
    for(i=0; i<npair; i++){
        k = (char *)(*(p+(2*i)));
        v = (char *)(*(p+(2*i+1)));

        if(NULL == (attr = gxml_get_attr(n, k))){
            attr = gxml_create_attr();
            if(NULL == attr) return -1;

            if(0 != create_key(k, strlen(k), attr)){
                free_gxml_node_attr(attr);
                return -1;
            }
            add_to_attrlist(attr, n);
        }
        if(attr->value)
            xmlfree(attr->value,attr->vl);
        attr->value = NULL;
        attr->vl = 0;
        if(0 != create_value(v, strlen(v), attr)){
            del_from_attrlist(attr, n);
            free_gxml_node_attr(attr);
            return -1;
        }
    }
    return 0;
}

void gxml_del_attr(gxmlhnd hnd, char *k){
    gxml_attr *attr = NULL;
    gxml_node *n = (gxml_node *)hnd;

    attr  = gxml_get_attr(n, k);

    if(!attr)
        return ;

    del_from_attrlist(attr, n);
    free_gxml_node_attr(attr);    
}

gxmlhnd gxml_add_child(gxmlhnd hnd, const char *key, const char *content){
    gxml_node *xml,*xmlchild; 
    gxml_node *n = (gxml_node *)hnd;

    if(!hnd || !key) return NULL;
    if(content && n->content) return NULL;    

    xml = gxml_create_node(key);
    if(!xml)
        return NULL;
    if(content){
        if(-1 == gxml_add_content(xml, content, strlen(content))){
            free_gxml_node(xml);
            return NULL;
        }
    }

    if(!n->child){
        n->child = xml;
        return (gxmlhnd)xml;
    }
    xmlchild = n->child;
    while(xmlchild->sibling){
        xmlchild = xmlchild->sibling;
    }
    xmlchild->sibling = xml;
    xml->parent = n;

    return (gxmlhnd)xml;            
}


int gxml_link2sibling(gxmlhnd hnd, gxmlhnd sibling){
    gxml_node *xml;
    gxml_node *r = (gxml_node *)hnd;
    gxml_node *e = (gxml_node *)sibling;
    
    if(!r || !e)
        return -1;
    xml = r;

    while(xml->sibling){
        xml = xml->sibling;
    }
    xml->sibling = e;
    e->parent = xml->parent;
    return 0;
}

int gxml_link2child(gxmlhnd hnd, gxmlhnd child){
    gxml_node *xml;
    gxml_node *r = (gxml_node *)hnd;
    gxml_node *c = (gxml_node *)child;

    if(!r || !c)
        return -1;
    xml = r->child;
    if(!xml){
        r->child = c;
    }else{
        while(xml->sibling){
            xml = xml->sibling;
        }
        xml->sibling = c;
    }
    c->parent = r;
    return 0;    
}

int gxml_dellink(gxmlhnd hnd){
    gxml_node *xml,*xmlpre;
    gxml_node *n = (gxml_node *)hnd;

    if(!n)
        return -1;
    if(!n->parent)
        return 0;
    xmlpre = xml = n->parent;
    xml = xml->child;
    if(xml == n){
        n->parent->child = xml->sibling;
        xml->parent = NULL;
        xml->sibling = NULL;
        return 0;
    }

    while(xml !=n ){
        xmlpre = xml;
        xml = xml->sibling;
    }
    assert(NULL != xml);

    xmlpre->sibling = xml->sibling;
    xml->parent = NULL;
    xml->sibling = NULL;
    return 0;            
}


char *gxml_getlable(gxmlhnd hnd){
    return hnd?((gxml_node *)hnd)->name:NULL;
}

gxmlhnd gxml_getparent(gxmlhnd hnd){
    return (gxmlhnd)(hnd?((gxml_node *)hnd)->parent:NULL);
}

void gxml_destroy_xmlattr(gxmlhnd hnd){
    gxml_node *n = (gxml_node *)hnd;
    if(!hnd) return;
    free_gxml_node_attr(n->attr);
    n->attr = NULL;
}

gxmlattrhnd gxml_get_firstattr(gxmlhnd hnd){
    return (gxmlattrhnd)(hnd?((gxml_node *)hnd)->attr:NULL);
}

gxmlattrhnd gxml_get_nextattr(gxmlattrhnd attr){
    return (gxmlattrhnd)(attr?((gxml_attr *)attr)->next:NULL);
}

char *gxml_get_attrkey(gxmlattrhnd attr){
    return attr?((gxml_attr *)attr)->key:NULL;
}

char *gxml_get_attrvalue(gxmlattrhnd attr){
    return attr?((gxml_attr *)attr)->value:NULL;
}

#if 0
int main(int argc, char*argv[])
{
    int id = 0;
    gxml_node *root = NULL;
#if 0    
    root = gxml_build_tree(XML_FILE_NAME);
#if 1
    if(-1 == gxml_foreach_specnode("media", 5, root, &id, printmedia))
    {
        printf("not find\r\n");
    }
#endif    

    //gxml_destroy_tree1(root);
    printf("----------------------------------------\r\n");
    print_xmltree(root);
    printf("----------------------------------------\r\n");
    //print_sibling(root);
    gxml_destroy_tree(root);
#endif
    printf("=========================\r\n");

    root = gxml_build_tree(TEST_WRITE_FILE);
#if 0
    if(-1 == gxml_foreach_specnode("media", 5, root, &id, printmedia))
    {
        printf("not find\r\n");
    }
#endif    

    printf("root is %p\r\n",root);

    gxml_node *nchild = gxml_get_child(root, NULL);
    printf("nchild is %s\r\n",nchild->name);
    gxml_add_attr(nchild, 2, "aaa", "bbb","idX","233");

    //free_gxml_node_attr(nchild->attr);
    //nchild->attr = NULL;

    gxml_save(root, TEST_WRITE_FILE);
    //gxml_destroy_tree1(root);
    printf("----------------------------------------\r\n");
    //print_xmltree(root);
    printf("----------------------------------------\r\n");
    //print_sibling(root);
    gxml_destroy_tree(root);
#if 0
    void *root_node = fw_conf_node_alloc("root", NULL);

    void *child = fw_conf_node_add_child(root_node, "func", NULL);

    void *fchild = fw_conf_node_add_child(child, "name", "config");
    void *nsibling = fw_conf_node_add_sibling(fchild, "value", "10000");

    fw_conf_save(root_node, "test.conf");

    root_node =  fw_conf_load("test.conf");

    child = fw_conf_node_child(root_node, "func");

    fchild = fw_conf_node_child(child, "name");

    fw_conf_node_delete(fchild);

    fw_conf_save(root_node, "test.new");

    fw_conf_node_free(root_node);
#endif    
    //root =  gxml_build_tree(DFT_MEDIAIP_NAME);
    //print_sibling(root);
    //gxml_destroy_tree(root);
    
    //root =  gxml_build_tree(DFT_MEDIAUSER_NAME);
    //print_sibling(root);
    //gxml_destroy_tree(root);

    return 0;
}
#endif


#ifdef __cplusplus
}
#endif

