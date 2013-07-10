#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys_func.h>
#include <sys_usr.h>
#include <list.h>

#define ROOT_ID  0
#define GUEST_ID 1
#define SYS_USR_MAX (SYS_USR_NUM + 2)

typedef struct usr_inner_t {
    struct list_head  list;
    char              pri_str[16];
    sys_usr_t         user;
}usr_inner_t;

static int              usr_cnt;
static usr_inner_t      usr_tbl[SYS_USR_MAX];
static struct list_head usr_list;
static struct list_head fre_list;

static sys_func_op_t usr_op;

static usr_inner_t *usr_alloc(char *name)
{
    usr_inner_t * usr = NULL;

    if(list_empty(&fre_list)){
        return NULL;
    }

    usr = list_first_entry(&fre_list, usr_inner_t, list);
    list_del(&usr->list);

    memset(usr, 0, sizeof(usr_inner_t));
    strncpy(usr->user.name, name, 15);
    list_add_tail(&usr->list, &usr_list);
    usr_cnt++;
    
    return usr;
}

static usr_inner_t *usr_find(char *name)
{
    usr_inner_t *usr_match = NULL;
    struct list_head *pos;

    __list_for_each(pos, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(strcmp(name, usr->user.name) == 0){
            usr_match = usr;
            break;
        }
    }

    return usr_match;
}

static int usr_set_pw(usr_inner_t *usr, char *pw)
{
    if(pw == NULL){
        return -1;
    }

    strncpy(usr->user.passwd, pw, 15);

    return 0;
}

static int usr_set_pri(usr_inner_t *usr, char *pri)
{
    int priority = atoi(pri);

    if(priority > 9999){
        priority = 0;
    }

    usr->user.priority = priority;

    return 0;
}

static int usr_info(void *udata, int size, char *buf)
{
    int off;
    int lft;
    int n;
    struct list_head *pos;

    off = 0;
    lft = size;

    n = snprintf(buf, lft, "Users: %d\n", usr_cnt);
    off += n;
    lft -= n;
    buf += n;

    __list_for_each(pos, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        n = snprintf(buf, lft, "%s\n", usr->user.name);
        off += n;
        lft -= n;
        buf += n;
    }
    
    return off;
}

static int usr_set(void *udata, sys_setting_t *set)
{
    int         i;
    int         ret = -1;
    usr_inner_t *usr;
    
    if(set->name == NULL){
        return -1;
    }

    _sys_func_dump_setting(set);
    usr = usr_find(set->name);
    if(usr == NULL){
        usr = usr_alloc(set->name);
    }
    if(usr == NULL){
        return -1;
    }
    
    for(i = 0; i < set->value_num; i++){
        sys_pair_t *value = &set->values[i];

        if(i == 0 && value->tag == NULL){
            ret = usr_set_pw(usr, value->val);
        }else
        if(0 == strcmp(value->tag, "pw")){
            ret = usr_set_pw(usr, value->val);
        }else if(0 == strcmp(value->tag, "pri")){
            ret = usr_set_pri(usr, value->val);
        }else{
            ret = -1;
        }

        if(ret != 0) return -1;
    }

    return 0;
}

static int usr_clr_all(void * udata)
{
    int i = 0;
    struct list_head *pos, *next;

    list_for_each_safe(pos, next, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(i++ > 1){
            list_del(&usr->list);
            list_add_tail(&usr->list, &fre_list);
            usr_cnt--;
        }
    }

    return 0;
}

static int usr_clr_byidx(void *udata, int idx)
{
    int i = 0;
    struct list_head *pos, *next;

    list_for_each_safe(pos, next, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(i++ == idx){
            list_del(&usr->list);
            list_add_tail(&usr->list, &fre_list);
            usr_cnt--;
            break;
        }
    }
    
    return 0;
}

static int usr_clr_bykey(void *udata, char *key)
{
    struct list_head *pos, *next;

    list_for_each_safe(pos, next, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(strcmp(usr->user.name, key) == 0){
            list_del(&usr->list);
            list_add_tail(&usr->list, &fre_list);
            usr_cnt--;
            return 0;
        }
    }
    
    return -1;
}

static int usr_get_byidx(void *udata, int idx, sys_setting_t *set)
{
    int i = 0;
    struct list_head *pos;

    __list_for_each(pos, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(i++ == idx){
            int n = snprintf(usr->pri_str, 15, "%d", usr->user.priority);
            usr->pri_str[n] = 0;
            set->name = usr->user.name;
            set->values[0].tag = "pw";
            set->values[0].val = usr->user.passwd;
            set->values[1].tag = "pri";
            set->values[1].val = usr->pri_str;
            set->value_num = 2;
            return 0;
        }
    }
    
    return -1;
}

static int usr_get_bykey(void *udata, char *key, sys_setting_t *set)
{
    struct list_head *pos;

    __list_for_each(pos, &usr_list){
        usr_inner_t *usr = list_entry(pos, usr_inner_t, list);

        if(strcmp(key, usr->user.name) == 0){
            int n = snprintf(usr->pri_str, 15, "%d", usr->user.priority);
            usr->pri_str[n] = 0;
            set->name = usr->user.name;
            set->values[0].tag = "pw";
            set->values[0].val = usr->user.passwd;
            set->values[1].tag = "pri";
            set->values[1].val = usr->pri_str;
            set->value_num = 2;
            return 0;
        }
    }
    
    return -1;
}

int sys_user_init(void)
{
    int i;

    INIT_LIST_HEAD(&usr_list);
    INIT_LIST_HEAD(&fre_list);
    
    for(i = 0; i < SYS_USR_MAX; i++){
        usr_inner_t *usr = &usr_tbl[i];

        memset(usr, 0, sizeof(usr_inner_t));
        INIT_LIST_HEAD(&usr->list);
        
        if(i == ROOT_ID){
            strncpy(usr->user.name,   "root", 16);
            strncpy(usr->user.passwd, "passwd", 16);
            usr->user.priority = 9999;
            list_add_tail(&usr->list, &usr_list);
        }else
        if(i == GUEST_ID){
            strncpy(usr->user.name,   "guest", 16);
            strncpy(usr->user.passwd, "", 16);
            usr->user.priority = 0;
            list_add_tail(&usr->list, &usr_list);
        }else{
            list_add_tail(&usr->list, &fre_list);
        }
    }
    usr_cnt = 2;

    usr_op.info    = usr_info;
    usr_op.set     = usr_set;
    usr_op.clr_all = usr_clr_all;
    usr_op.clr_byidx = usr_clr_byidx;
    usr_op.clr_bykey = usr_clr_bykey;
    usr_op.get_byidx = usr_get_byidx;
    usr_op.get_bykey = usr_get_bykey;
    
    return sys_func_register("/sys/usr", SYS_USR_MAX, &usr_op, NULL);
}

sys_usr_t *sys_user_def(void)
{
    return &usr_tbl[GUEST_ID].user;
}

sys_usr_t *sys_user_find(char *usr_info)
{
    char *name;
    char *pw;
    
    usr_inner_t *usr = NULL;

    pw = strchr(usr_info, '.');
    if(pw == NULL){
        return NULL;
    }
    
    name = usr_info;
    *pw++ = 0;
    
    usr = usr_find(name);
    if(usr == NULL){
        return NULL;
    }
    
    if(strcmp(pw, usr->user.passwd) == 0){
        return &usr->user;
    }

    return NULL;
}


