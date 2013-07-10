#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <list.h>

#include <sys_func.h>

#define FUNC_MAX        128
#define FUNC_ENTRY_DEEP (8)
#define FUNC_ENTRY_MAX  (FUNC_MAX * 2)
#define FUNC_NAME_SZ    32


typedef struct _func_t {
    int             capacity;
    sys_func_op_t   op;
    void           *udata;
}func_t;

typedef struct _func_entry_t {
    struct _func_entry_t *parent;
    struct list_head      sibling;
    struct list_head      child;
    int                   version;
    char                  name[FUNC_NAME_SZ];
    func_t               *func;
}func_entry_t;

static int          func_ver  = 0;
static int          func_num  = 0;
static func_t       funcs[FUNC_MAX];
static int          entry_num = 0;
static func_entry_t entrys[FUNC_ENTRY_MAX];

static func_entry_t entry_root;

static func_t *func_pop(int capacity, const sys_func_op_t *fop, void *udata)
{
    func_t *func;
    
    if(func_num >= FUNC_MAX){
        return NULL;
    }
    
    func = &funcs[func_num++];

    func->capacity = capacity;
    func->op    = *fop;
    func->udata = udata;

    return func;
}

static void func_push(void)
{
    if(func_num > 0){
        func_num--;
    }
}

static func_entry_t *entry_pop(char *name)
{
    func_entry_t *entry;
    if(entry_num >= FUNC_ENTRY_MAX){
        return NULL;
    }
    
    entry = &entrys[entry_num++];

    strncpy(entry->name, name, FUNC_NAME_SZ);
    entry->parent = NULL;
    entry->func   = NULL;
    entry->version= 0;
    INIT_LIST_HEAD(&entry->sibling);
    INIT_LIST_HEAD(&entry->child);
    
    return entry;
}

static void entry_push(void)
{
    if(entry_num > 0){
        entry_num--;
    }
}

static int entry_scan(func_entry_t *entry, int (*cb)(void *, void *), void *uarg)
{
    int               skip;
    struct list_head *pos;
    
    skip = cb(entry, uarg);
    if(skip || entry->func){
        return skip;
    }  

    __list_for_each(pos, &entry->child){
        func_entry_t *cur_entry;

        cur_entry = container_of(pos, func_entry_t, sibling);
        skip = entry_scan(cur_entry, cb, uarg);
        
        if(skip){
            break;
        }
    }
    
    return skip;
}

static func_entry_t *entry_find(const char *path)
{
    int  i;
    char tmp_buf[1024];
    char *name;
    char *ctx;
    func_entry_t *entry;
    struct list_head *pos;

    //name check and copy
    for(i = 0; i < 1024 - 1; i++){
        char ch = path[i];
        
        if(ch == 0){
            break;
        }
        
        if(isspace(ch)){
            return NULL;
        }
        
        tmp_buf[i] = ch;
    }
    tmp_buf[i] = 0;

    //find 
    name  = strtok_r(tmp_buf, "./\\", &ctx);
    entry = &entry_root;
    for( ; name; name = strtok_r(NULL, "./\\", &ctx)){
        int gono = 0;
        
        __list_for_each(pos, &entry->child){
            func_entry_t *cur_entry;

            cur_entry = container_of(pos, func_entry_t, sibling);
            if(strcmp(name, cur_entry->name) == 0){
                entry = cur_entry;
                gono = 1;
                break;
            }
        }

        if(!gono){
            entry = NULL;
            break;
        }
    }

    return entry;
}

static func_entry_t *entry_add(const char *path)
{
    int  i;
    int  deep = 0;
    char tmp_buf[1024];
    char *name;
    func_entry_t *parent;
    func_entry_t *new_entry;
    struct list_head *pos;

    //name check and copy
    for(i = 0; i < 1024 - 1; i++){
        char ch = path[i];
        
        if(ch == 0){
            break;
        }else if(isspace(ch)){
            return NULL;
        }else{
            tmp_buf[i] = ch;
        }
    }
    tmp_buf[i] = 0;

    //find hook point
    name   = strtok(tmp_buf, "./\\");
    parent = &entry_root;
    for( ; name; name = strtok(NULL, "./\\")){
        int gono = 0;
        
        __list_for_each(pos, &parent->child){
            func_entry_t *cur_entry;

            cur_entry = container_of(pos, func_entry_t, sibling);
            if(strcmp(name, cur_entry->name) == 0){
                parent = cur_entry;
                gono = 1;
                break;
            }
        }
        
        deep++;
        if(!gono){
            break;
        }
    }

    if(name == NULL){
        //path already exist!
        return NULL;
    }

    if(parent->func){
        //path invalid !
        return NULL;
    }

    //add route
    new_entry = NULL;
    for( ; name && deep < FUNC_ENTRY_DEEP; name = strtok(NULL, "./\\")){
        new_entry = entry_pop(name);

        if(new_entry == NULL){
            //Todo: release pop
            return NULL;
        }

        new_entry->parent = parent;
        list_add_tail(&new_entry->sibling, &parent->child);

        parent = new_entry;
        
        deep++;
    }

    if(name){
        //out of deep
        return NULL;
    }else{
        return new_entry;
    }
}

static int setting_parse(char *setting, sys_setting_t *fset)
{
    char *name;
    char *values;
    char *value;
    char *assign;
    char *ctx;
    int   i;

    assign = strchr(setting, '=');
    if(assign == NULL){
        name   = NULL;
        values = setting;
    }else{
        name   = strtok(setting, " \t=");
        values = assign + 1;
    }
    fset->name = name;

    value = strtok_r(values, ",", &ctx);
    
    for(i = 0; value && i < FUNC_VALUE_MAX; i++, value = strtok_r(NULL, ",", &ctx)){       
        assign = strchr(value, ':');
        
        if(assign){
            fset->values[i].tag = strtok(value, " \t:");
            fset->values[i].val = strtok(assign + 1, " \t");
        }else{
            fset->values[i].tag = NULL;
            fset->values[i].val = strtok(value, " \t");
        }
    }
    fset->value_num = i;

    return 0;
}

int sys_func_init(void)
{
    entry_root.parent = NULL;
    INIT_LIST_HEAD(&entry_root.sibling);
    INIT_LIST_HEAD(&entry_root.child);
    entry_root.func   = NULL;
    strncpy(entry_root.name, "", FUNC_NAME_SZ);
    
    return 0;
}

int sys_func_deinit(void)
{
    return 0;
}

int sys_func_register(const char *name, int capacity,
                      const sys_func_op_t *fop, void *udata)
{
    func_t       *func;
    func_entry_t *entry;

    func  = func_pop(capacity, fop, udata);
    if(func == NULL){
        return -1;
    }
    
    entry = entry_add(name);
    if(entry == NULL){
        func_push();
        return -1;
    }

    entry->func = func;
    
    return 0;
}

void *sys_func_entry_root(void)
{
    return &entry_root;
}

void *sys_func_get_entry(const char *name)
{
    return (void *)entry_find(name);
}

int  sys_func_entry_scan(void *func_entry, int (*cb)(void *, void *), void *uarg)
{
    func_entry_t *entry = (func_entry_t *)func_entry;
    
    return entry_scan(entry, cb, uarg);
}

int   sys_func_entry_get_name(void *func_entry, int size, char *buf)
{
    int           len;
    int           off = 0;
    int           i;
    func_entry_t *route[FUNC_ENTRY_DEEP];
    func_entry_t *entry;
    
    //build entry route
    route[0] = (func_entry_t *)func_entry;
    for(i = 1; i < FUNC_ENTRY_DEEP; i++){
        route[i] = route[i - 1]->parent;
        
        if(route[i] == NULL){
            break;
        }
    }

    if(i > FUNC_ENTRY_DEEP){
        assert(0); //out of deep!
        return 0;
    }

    //build entry route name
    for(; i > 0; i--){
        entry = route[i - 1];
        
        len = snprintf(&buf[off], size - off - 2, "%s", entry->name);
        off += len;

        if(entry->func == NULL){
            buf[off++] = '/';
        }else{
            break;
        }
    }
    //buf[off++] = '\n';

    return off;
}

int sys_entry_ver(void *func_entry)
{
    func_entry_t *entry;
    
    entry = (func_entry_t *)func_entry;

    return entry->version;
}

int   sys_func_type(void *func_entry)
{
    func_entry_t *entry;
    
    entry = (func_entry_t *)func_entry;

    if(entry->func){
        return SYS_FUNC_DICT;
    }else{
        return SYS_FUNC_NULL;
    }
}

int   sys_func_capacity(void *func_entry)
{
    func_entry_t *entry;
    
    entry = (func_entry_t *)func_entry;

    if(entry->func == NULL){
        return 0;
    }

    return entry->func->capacity;
}

int   sys_func_info(void *func_entry, int size, char *buf)
{   
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    
    entry = (func_entry_t *)func_entry;
    
    if(entry->func == NULL){
        return -1;
    }

    func = entry->func;
    fop  = &func->op;

    if(fop->info == NULL){
        return 0;
    }
    
    return fop->info(func->udata, size, buf);
}

int  sys_func_clr(void *func_entry)
{
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    int             i;
    int             ret = 0;
    
    entry = (func_entry_t *)func_entry;
    if(entry->func == NULL){
        return -1;
    }

    func = entry->func;
    fop  = &func->op;
    if(fop->clr_all){
        ret = fop->clr_all(func->udata);
        goto UPDATE_VER;
    }

    if(fop->clr_byidx == NULL){
        return -1;
    }

    for(i = 0; i < func->capacity; i++){
        if(0 != fop->clr_byidx(func->udata, i)){
            return -1;
        }
    }

UPDATE_VER:
    if(ret == 0){
        int ver  = ++func_ver;
        while(entry){
            entry->version = ver;
            entry = entry->parent;
        }
    }

    return 0;
}

int sys_func_clr_bykey(void *func_entry, char *key)
{
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    int              ret;
    
    entry = (func_entry_t *)func_entry;
    if(entry->func == NULL){
        return -1;
    }

    func = entry->func;
    fop  = &func->op;
    if(fop->clr_bykey == NULL){
        return -1;
    }

    ret = fop->clr_bykey(func->udata, key);
    if(ret == 0){
        int ver  = ++func_ver;
        while(entry){
            entry->version = ver;
            entry = entry->parent;
        }
    }
    return ret;
}

int  sys_func_set(void *func_entry, int len, char *setting)
{
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    sys_setting_t   fset;
    int             ret;
    
    entry = (func_entry_t *)func_entry;
    if(entry->func == NULL){
        return -1;
    }
    
    func = entry->func;
    fop  = &func->op;
    if(fop->set == NULL){
        return -1;
    }

    setting_parse(setting, &fset);

    ret = fop->set(func->udata, &fset);

    if(ret == 0){
        int ver  = ++func_ver;
        while(entry){
            entry->version = ver;
            entry = entry->parent;
        }
    }
    
    return ret;
}

int  sys_func_get_byidx(void *func_entry, int idx, int size, char *setting)
{
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    sys_setting_t    set;
    char            *buf;
    int              lft;
    int              len;
    int              i;
    
    entry = (func_entry_t *)func_entry;
    if(entry->func == NULL){
        return -1;
    }

    func = entry->func;
    if(idx >= func->capacity){
        return -1;
    }
    
    fop  = &func->op;
    if(fop->get_byidx == NULL){
        return -1;
    }

    memset(&set, 0, sizeof(set));
    if(0 != fop->get_byidx(func->udata, idx, &set)){
        return -1;
    }

    if(set.name == NULL || set.value_num == 0){
        return 0;
    }

    lft = size;
    buf = setting;
    
    if(set.name){
        len = snprintf(buf, lft, "%s=", set.name);
        buf += len;
        lft -= len;
    }

    for(i = 0; i < set.value_num && lft > 0; i++){
        sys_pair_t *pair = &set.values[i];
        
        if(pair->tag){
            len = snprintf(buf, lft, "%s:", pair->tag);
            buf += len;
            lft -= len;
        }
        if(pair->val){
            len = snprintf(buf, lft, "%s", pair->val);
            buf += len;
            lft -= len;
        }
        if(lft > 0 && i < set.value_num - 1){
            *buf++ = ',';
            lft--;
        }
    }

    if(lft){
        *buf = '\n';
        lft--;
    }

    return size - lft;
}

int  sys_func_get_bykey(void *func_entry, char *key, int size, char *setting)
{
    func_entry_t    *entry;
    func_t          *func;
    sys_func_op_t   *fop;
    sys_setting_t    set;
    char            *buf;
    int              lft;
    int              len;
    int              i;
    
    entry = (func_entry_t *)func_entry;
    if(entry->func == NULL){
        return -1;
    }

    func = entry->func;
    fop  = &func->op;
    if(fop->get_bykey == NULL){
        return -1;
    }

    memset(&set, 0, sizeof(set));
    if(0 != fop->get_bykey(func->udata, key, &set)){
        return -1;
    }
    
    if(set.name == NULL || set.value_num == 0){
        return 0;
    }

    lft = size;
    buf = setting;
    
    if(set.name){
        len = snprintf(buf, lft, "%s=", set.name);
        buf += len;
        lft -= len;
    }

    for(i = 0; i < set.value_num && lft > 0; i++){
        sys_pair_t *pair = &set.values[i];
        
        if(pair->tag){
            len = snprintf(buf, lft, "%s:", pair->tag);
            buf += len;
            lft -= len;
        }
        if(pair->val){
            len = snprintf(buf, lft, "%s", pair->val);
            buf += len;
            lft -= len;
        }
        if(lft > 0 && i < set.value_num - 1){
            *buf++ = ',';
            lft--;
        }
    }

    if(lft){
        *buf = '\n';
        lft--;
    }

    return size - lft;
}

void _sys_func_dump_setting(sys_setting_t *setting)
{
    int i;

    printf("name: %s\n", setting->name ? setting->name : "NV");
    for(i = 0; i < setting->value_num; i++){
        sys_pair_t *value = &setting->values[i];
        printf("tag:%s-%s\n", value->tag ? value->tag : "NV",
                          value->val ? value->val : "NV");
    }
}

