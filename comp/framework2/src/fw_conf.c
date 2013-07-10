
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
#include "public.h"
#include "gatomic.h"

#define VISIABLE_CHAR_START  32
#define VISIABLE_CHAR_END    126

#define KBYTE 1024

#define NUM8B (KBYTE * 8)
#define NUM16B (KBYTE * 8)
#define NUM32B (KBYTE * 4)
#define NUM64B (KBYTE * 4)
#define NUM128B (KBYTE * 2)
#define NUM1024B (KBYTE * 1)

//#define GXMLFREE FREE
//#define GXMLMALLOC MALLOC
#define MAX_POW 64

int g_statlen[2048];



struct _vbuf_head{
    void *pnext;
};

struct buf_pool_tag{
    int spinlock;
    void *head;
    void *tail;
    int maxnum;
    int left;
    int blk;
    struct _vbuf_head next;
};

struct _binary_head{
    void *pstart;
    void *pend;
    struct buf_pool_tag *pool;
};

static struct buf_pool_tag g_bufpool[MAX_POW];
static struct _binary_head g_binary_poolhead[MAX_POW];
static int g_binary_idx = 0;

typedef struct _gxml_attr
{
    struct _gxml_attr *next;
    char *key;
    char *value;
}gxml_attr;

typedef struct _gxml_node
{
    char *name;
    char *content;
    gxml_attr *attr;
    struct _gxml_node *child;
    struct _gxml_node *parent;
    struct _gxml_node *sibling;
}gxml_node;

typedef int (*gxml_func)(void *arg, gxml_node *root);

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


static int get_pow(int n)
{
    int i=0,m = n;

    if(n<=0)
        return -1;
        
    while(i<MAX_POW)
    {
        if(0 == (m=(m>>1)))
        {
            break;
        }
        i++;        
    }

    if(MAX_POW == i)
        return -1;
    
    if((~(1<<i)) & n)
        i++;
    
    return i;
}

static void _gxml_bytefree(struct _vbuf_head *head, struct _vbuf_head *node)
{
    struct _vbuf_head *p = (struct _vbuf_head *)head->pnext;

    head->pnext = node;
    node->pnext = p;    
}



static int gxml_init_mempool(int byte, int num)
{
    struct buf_pool_tag *pool;
    int pow,i,blk;
    char *pnext;
    struct _binary_head *pbinary;
    
    pow = get_pow(byte);
    if(-1 == pow)
        return -1;
    pool = &g_bufpool[pow];

    blk = 1 << pow;

    if(blk < sizeof(void *))
        return-1;
    pool->head = MALLOC(blk * num);

    if(!pool->head)
        return -1;
    pbinary = (struct _binary_head *)&g_binary_poolhead[g_binary_idx++];
    pbinary->pool = pool;
    pbinary->pstart = pool->head;

    pool->left = num;
    pool->maxnum = num;
    pool->blk = blk;
    pool->spinlock = 0;

    pnext = (char *)pool->head;
    for(i=0;i<num; i++)
    {
        _gxml_bytefree(&pool->next, (struct _vbuf_head *)pnext);
        pnext += blk;
    }
    pool->tail = (void *)pnext;
    pbinary->pend = pool->tail;
    return 0;        
}

static void *GXMLMALLOC(int size)
{
    int pow,i;
    struct buf_pool_tag *pool;
    struct _vbuf_head *pbuf = NULL;
    
    pow = get_pow(size);

    if(pow < 0 || pow >= MAX_POW)
        return NULL;

    for(i=pow; i<MAX_POW; i++)
    {
        pool = &g_bufpool[i];

        atomic_lock(&pool->spinlock);

        if(pool->left > 0)
        {
            pbuf = (struct _vbuf_head *)pool->next.pnext;
            pool->next.pnext = pbuf->pnext;
            pool->left --;
        }
        atomic_unlock(&pool->spinlock);
        if(pbuf)
        {
            //g_statlen[pool->blk] ++;
            break;
        }
    }
    
    return pbuf;    
    
}

static struct buf_pool_tag *pool_bsearch(void *p)
{
    int off,head,tail;
    
    struct _binary_head *bhead;
    struct buf_pool_tag *pool = NULL;

    head = 0;
    tail = g_binary_idx - 1;

    do{
        off = (head + tail)/2;
        bhead = &g_binary_poolhead[off];
        if(p >= bhead->pstart && p < bhead->pend)
        {
            pool = bhead->pool;
            break;
        }
        if(p<bhead->pstart)
        {
            head = off+1;
        }
        else
        {
            tail = off-1;
        }
    }while(head <= tail);

    return pool;
}

void GXMLFREE(void *p)
{
    struct buf_pool_tag *pool;
    
    pool = pool_bsearch(p);

    if(!pool)
        return;

    //g_statlen[pool->blk] --;

    atomic_lock(&pool->spinlock);
    _gxml_bytefree(&pool->next, (struct _vbuf_head *)p);
    pool->left ++;
    atomic_unlock(&pool->spinlock);
    
}


static int gopenfile(const char *filename)
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

static int findchar(char c, char *buf, int buflen)
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
    gxml_node *xml = (gxml_node *)GXMLMALLOC(galign(sizeof(gxml_node),8));

    if(NULL == xml)
    {
        printf("%s malloc error\r\n",__FUNCTION__);
        return NULL;
    }

    memset(xml,0,sizeof(gxml_node));

    return xml;
}

static void free_gxml(gxml_node *xml)
{
    if(NULL == xml)
    {
        return ;
    }
    GXMLFREE(xml);
}


static void free_gxml_node(gxml_node *xml)
{
    gxml_attr *attr = NULL;
    gxml_attr *attrnext = NULL;

    if(!xml) return ;

    if(NULL != xml->attr)
    {
        attr = xml->attr;
        attrnext = attr;
        while(attr)
        {
            attrnext = attr->next;
            GXMLFREE(attr->key);
            GXMLFREE(attr->value);
            GXMLFREE(attr);
            attr = attrnext;
        }
        xml->attr = NULL;
    }
    if(NULL != xml->content)
    {
        GXMLFREE(xml->content);
        xml->content = NULL;
    }
    if(NULL != xml->name)
    {
        GXMLFREE(xml->name);
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
        GXMLFREE(xml);
        return NULL;
    }

    xml->name = (char *)GXMLMALLOC(galign(len,8));

    if(NULL == xml->name)
    {
        printf("malloc name %s failed\r\n", buf);
        GXMLFREE(xml);
        return NULL;
    }

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

    p->name = (char *)GXMLMALLOC(galign(key_len, 8));
    if(NULL == p->name)
    {
        printf("malloc key %s failed\r\n", key);
        free_gxml(p);
        return -1;
    }
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
        if((unsigned char)buf[nloop] < VISIABLE_CHAR_START) // || buf[nloop] >VISIABLE_CHAR_END)
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

static void add_to_attrlist(gxml_attr *newattr, gxml_node *n)
{
    gxml_attr *attr;

    if(NULL == n->attr)
    {
        n->attr = newattr;
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
        attr->next = newattr;
    }
}

static int create_value(char *buf, int nc, gxml_attr *attr)
{
    attr->value = (char *)GXMLMALLOC(galign(nc, 8));
    if(NULL ==attr->value)
    {
        printf("malloc value %s failed\r\n",buf);
        return -1;
    }
    memcpy(attr->value, buf, nc);
    attr->value[nc] = '\0';
    return 0;
}

static int create_key(char *buf, int nc, gxml_attr *attr)
{
    attr->key = (char *)GXMLMALLOC(galign(nc, 8));
    if(NULL ==attr->key)
    {
        printf("carete key %s failed\r\n",buf);
        return -1;
    }
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
        xmlattr = (gxml_attr *)GXMLMALLOC(sizeof(gxml_attr));
        if(NULL == xmlattr)
        {
            printf("malloc xmlattr failed\r\n");
            return -1;
        }

        xmlattr->value = NULL;
        xmlattr->next = NULL;
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

static void gxml_destroy_tree(gxml_node * root);
static int  gxml_add_content(gxml_node *n, const char *content, int clen);

static int gxml_push(q_xml_buf **xml_q,char *data, int dlen, gxml_node *xml)
{
    q_xml_buf *buf = NULL;

    if(NULL == data || NULL == xml)
    {
        return -1;
    }

    buf = (q_xml_buf *)GXMLMALLOC(galign(sizeof(q_xml_buf), 8));
    if(NULL == buf)
    {
        printf("malloc q_xml_buf buffer  failed\r\n");
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

    GXMLFREE(buf);

    return 0;
}

static void gxml_destroy_stack(q_xml_buf **xml_buf)
{
    char *data;
    int len;
    gxml_node *xml;
    while(0 == gxml_pop(xml_buf,&data,&len,&xml));
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

static int do_build_childxmltree(q_xml_buf **xml_buf, char *buf, int buflen, gxml_node *root)
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

static gxml_node* gxml_build_tree(const char *filename)
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

    if(NULL == (fbuf = (char *)malloc(galign(gfstat.st_size+8,8))))
    {
        printf("malloc buffer  failed\r\n");
        close(fd);
        return NULL;
    }

    if(-1== gxml_readfile(fd,fbuf,gfstat.st_size))
    {
        printf("readfile error\n");
        free(fbuf);
        close(fd);
        return NULL;
    }

    memset(fbuf+gfstat.st_size,0,8);
    if(-1 == gxml_parser(fbuf, gfstat.st_size+8, &root))
    {
        printf("gxml_parser error\n");
        free(fbuf);
        close(fd);
        return NULL;
    }
    free(fbuf);
    close(fd);
    
    return root;
}

static void gxml_destroy_tree(gxml_node * root)
{
    gxml_node *top = root;
    gxml_node *head = root;
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


#define GXML_KEY "sched_host"
#if 0
int printmedia(void *id,gxml_node *xml)
{
    int mid = 0;

    if(xml->attr)
    {
        mid = atoi(xml->attr->value);
        if(-1 == gxml_foreach_specnode(GXML_KEY, strlen(GXML_KEY), xml->child, &mid, print_ip_content))
        {
            return -1;
        }
    }
    return 0;
}

int gxml_traverse_all(gxml_node *root, char *key,void *arg,int (*func)(void *arg,gxml_node *node))
{
    int *media_id = (int *)arg;

    if(root == NULL) {
        return -1;
    }

    if(strcmp(root->name,"media") == 0) {
       *media_id = atoi(root->attr[0].value);
    }

    if(strcmp(root->name,key) == 0) {
       func((void *)media_id, root);
    }

    gxml_traverse_all(root->child,key,(void *)media_id, func);
    gxml_traverse_all(root->sibling, key, (void *)media_id,func);

    return 0;
}
#endif
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

    if(write(fd, "<?xml version=\"1.0\"?>\r\n", sizeof("<?xml version=\"1.0\"?>\r\n")-1)<0){
        perror("write");
        return -1;
    }

    xml = root;

    while(xml){
        if(-1 == gxml_getprevtag(deep,buf, sizeof(buf), &len,xml)){
            goto errsave;
        }
        len = write(fd, buf, len);
        if(len < 0){
            perror("write");
        }
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
    perror("?");
    gxml_destroy_stack(&sxml);
    return -1;
}

static int gxml_save(gxml_node *root, const char *filename){
    int fd;

    if(!root || !filename){
        return -1;
    }

    fd = open(filename, O_RDWR|O_TRUNC|O_CREAT, 0666);
    if(fd < 0 ){
        perror("open");
        return -1;
    }

    if(-1 == gxml_write2file(fd, root->child)){
        printf("write 2 file error\r\n");
    }

    close(fd);

    return 0;
}

static gxml_node *_gxml_getsibling_node(gxml_node *n, const char *key){
    int len = strlen(key);
    while(n){
        //printf("cmp: %s, %s\n", n->name, key);
        if(strlen(n->name) == len){

            if(0 == memcmp(n->name, key, len))
                return n;
        }
        n = n->sibling;
    }
    return NULL;
}

static gxml_node *gxml_get_child(gxml_node *n, const char *key){
    gxml_node *child = n?(n->child):NULL;

    if(NULL == key)
        return child;

    return _gxml_getsibling_node(child, key);
}

static gxml_node *gxml_next_sibling(gxml_node *n, const char *key){
    gxml_node *next = n?(n->sibling):NULL;

    if(NULL == key){
        return next;
    }
    return _gxml_getsibling_node(next, key);
}

static char *gxml_get_content(gxml_node *n){
    return n?n->content:NULL;
}

static gxml_node *gxml_create_node(const char *key){
    gxml_node *xml = NULL;
    if(!key)
        return NULL;
    if( 0 != create_xml(key, strlen(key), &xml)){
        return NULL;
    }
    return xml;
}

static int gxml_add_content(gxml_node *n, const char *content, int clen){
    if(!n || !content) return -1;

    if(n->content){
        GXMLFREE(n->content);
        n->content = NULL;
    }

    n->content = (char *)GXMLMALLOC(galign(clen, 8));
    if(NULL == n->content)
    {
        printf("malloc content %s  failed\r\n",content);
        return -1;
    }
    memcpy(n->content, content, clen);
    n->content[clen] = '\0';

    return 0;
}

static gxml_node *gxml_add_child(gxml_node *n, const char *key, const char *content){
    gxml_node *xml,*xmlchild;

    if(!n || !key) return NULL;
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
        return xml;
    }
    xmlchild = n->child;
    while(xmlchild->sibling){
        xmlchild = xmlchild->sibling;
    }
    xmlchild->sibling = xml;
    xml->parent = n;

    return xml;
}


static int gxml_link2sibling(gxml_node *r, gxml_node *e){
    gxml_node *xml;
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

static int gxml_link2child(gxml_node *r, gxml_node *c){
    gxml_node *xml;

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

static int gxml_dellink(gxml_node *n){
    gxml_node *xml,*xmlpre;

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


static char *gxml_getlable(gxml_node *n){
    return n?n->name:NULL;
}

static gxml_node *gxml_getparent(gxml_node *n){
    return n?n->parent:NULL;
}

int fw_conf_init()
{
    int i;

    for(i=0; i<sizeof(g_statlen)/sizeof(int); i++)
    {
        g_statlen[i] = 0;
    }

    memset(g_binary_poolhead,0, sizeof(g_binary_poolhead));

    memset(g_bufpool, 0, sizeof(g_bufpool));
    gxml_init_mempool(8, NUM8B);
    gxml_init_mempool(16, NUM16B);
    gxml_init_mempool(32, NUM32B);
    gxml_init_mempool(64, NUM64B);
    gxml_init_mempool(128, NUM128B);
    gxml_init_mempool(1024, NUM1024B);
    
    xmlno = 0;
    return 0;
}

int fw_conf_deinit()
{
    int i;
    struct _binary_head *bh;
    struct buf_pool_tag *pool;
    
    for(i=0; i<g_binary_idx; i++)
    {
        bh = (struct _binary_head *)&g_binary_poolhead[i];
        pool = bh->pool;
        if(!pool)
            continue;
        FREE(pool->head);
        memset(pool, 0, sizeof(struct buf_pool_tag));
    }
    return 0;
}

void* fw_conf_load(const char *filename)
{
    void *p = gxml_build_tree(filename);
    return p;
}

int fw_conf_save(void *root, const char *conf)
{
    return gxml_save((gxml_node *)root, conf);
}

void *fw_conf_node_alloc(const char *label, const char *content)
{
    void *n;
    n = gxml_create_node(label);
    if(!n)
        return NULL;

    if(content){
        if(-1 == gxml_add_content((gxml_node *)n, content, strlen(content))){
            gxml_destroy_tree((gxml_node *)n);
            return NULL;
        }
    }
    return n;
}

void  fw_conf_node_free(void *node)
{
    gxml_destroy_tree((gxml_node *)node);
}

void fw_conf_node_delete(void *node)
{
    if(0 != gxml_dellink((gxml_node *)node))
        return;
    gxml_destroy_tree((gxml_node *)node);
}

char *fw_conf_node_label(void *node)
{
    return gxml_getlable((gxml_node *)node);
}

char *fw_conf_node_content(void *n)
{
    return gxml_get_content((gxml_node *)n);
}

void *fw_conf_node_parent(void *node)
{
    return gxml_getparent((gxml_node *)node);
}

void* fw_conf_node_child(void *n, const char *key)
{
    return gxml_get_child((gxml_node *)n, key);
}

void *fw_conf_node_next(void *n, const char *key)
{
    return gxml_next_sibling((gxml_node *)n, key);
}

void *fw_conf_node_add_child(void *node, const char *label, const char *content)
{
    return gxml_add_child((gxml_node *)node, label, content);
}

int fw_conf_node_add_child_node(void *node, void *child_node)
{
    return gxml_link2child((gxml_node *)node, (gxml_node *)child_node);
}

void *fw_conf_node_add_sibling(void *node, const char *label, const char *content)
{
    void *n = fw_conf_node_alloc(label, content);
    if(!n)
        return NULL;
    if(0 != gxml_link2sibling((gxml_node *)node, (gxml_node *)n)){
        gxml_destroy_tree((gxml_node *)n);
        return NULL;
    }
    return n;
}

int fw_conf_node_add_sibling_node(void *node, void *sibling_node)
{
    return gxml_link2sibling((gxml_node *)node, (gxml_node *)sibling_node);
}

int stat_gxmlmem(char *buf, int len)
{
    int l = len;
    int i;


    for(i=0; i<2048; i++)
    {
        if(g_statlen[i]>0)
        {
            l -= snprintf(buf+(len-l), l, "i: %d, num: %d\r\n",i, g_statlen[i]);        
        }
    }    
    return len-l;
}

void fw_conf_outprint(void *node)
{
    gxml_node *n = (gxml_node *)node;

    printf("label : %s\r\n", n->name);
    if(n->content)
        printf("content : %s\r\n", n->content);
    if(n->child)    
    {
        printf("child : ");
        fw_conf_outprint(n->child);
    }
    if(n->sibling)
    {
        printf("sibling : ");
        fw_conf_outprint(n->sibling);
    }    
}


