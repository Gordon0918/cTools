/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      fw_fcmd.c
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include <fw_fobj.h>
#include <fw_intf.h>
#include <fw_conf.h>


/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/


/******************************************************************************
 * Types & Defines                                                            *
 *****************************************************************************/
typedef struct _fcmd_hndl_t {
    int   type; // 0: func,  1: func.attr,  2: func.proc
                //10: inst, 11: inst.attr, 12: inst.proc
    int   func;
    void *inst;
    char *attr;
    char *proc;
}fcmd_hndl_t;

/******************************************************************************
 * Local  variable                                                            *
 *****************************************************************************/


/******************************************************************************
 * Local  function                                                            *
 *****************************************************************************/
static int fcmd_inst2str(void *inst, char *buf, int size)
{
    return snprintf(buf, size, "Inst@%p", inst);
}

static int fcmd_str2inst(char *str, void **inst, char **attr, char **proc)
{
    int  off = 0;
    
    char *hex = str + 7;
    unsigned long prefix = 0;
    char         *suffix = NULL;
    
    
    if(memcmp("Inst@0x", str, 7) != 0){
        return -1;
    }
    
    while(off < (sizeof(inst) * 2)){
        int v = hex[off++];

        if(v >= '0' && v <= '9'){
            prefix = (prefix << 4) + v - '0';
        }else if(v >= 'a' && v <= 'f'){
            prefix = (prefix << 4) + v - 'a' + 10;
        }else if(v >= 'A' && v <= 'F'){
            prefix = (prefix << 4) + v - 'A' + 10;
        }else if(v == 0){
            break;
        }else if(v == '.'){
            suffix = hex + off;
            break;
        }else{
            return -1;
        }
    }

    if(off < 4){
        return -1;
    }else{
        *inst = (void *)prefix;
        if(suffix){
            if(suffix[0] == '@'){
                *attr = &suffix[1];
                *proc = NULL;
            } else {
                *attr = NULL;
                *proc = suffix;
            }
        }else{
            *attr = NULL;
            *proc = NULL;
        }
        return 0;
    }
}

static int fcmd_fobj2str(int fobj, char *buf, int size)
{
    return snprintf(buf, size, "Fobj@%d", fobj);
}

static int fcmd_str2fobj(char *str, int *fobj, char **attr, char **proc)
{
    int  off = 0;
    
    char *dec = str + 5;
    int  prefix = 0;
    char *suffix = NULL;
    
    
    if(memcmp("Fobj@", str, 5) != 0){
        return -1;
    }
    
    while(off < (sizeof(int) * 2)){
        int v = dec[off++];

        if(v >= '0' && v <= '9'){
            prefix = (prefix * 10) + v - '0';
        }else if(v == 0){
            break;
        }else if(v == '.'){
            suffix = dec + off;
            break;
        }else{
            return -1;
        }
    }

    if(off < 2){
        return -1;
    }else{
        *fobj = prefix;
        if(suffix){
            if(suffix[0] == '@'){
                *attr = &suffix[1];
                *proc = NULL;
            } else {
                *attr = NULL;
                *proc = suffix;
            }
        }else{
            *attr = NULL;
            *proc = NULL;
        }
        return 0;
    }
}

static int fcmd_str2hndl(char *str, fcmd_hndl_t *hndl)
{
    int   wc;
    char *w[4];
    char *ctx;
    int   type = -1;
    int   func = -1;
    void *inst = NULL;
    char *attr = NULL;
    char *proc = NULL;
    
    if(0 == fcmd_str2fobj(str, &func, &attr, &proc)){
        hndl->func = func;
        hndl->inst = NULL;
        if(attr != NULL){
            hndl->type = 1;
            hndl->attr = attr;
            hndl->proc = NULL;
        }else if(proc != NULL){
            hndl->type = 2;
            hndl->attr = NULL;
            hndl->proc = proc;
        }else{
            hndl->type = 0;
            hndl->attr = NULL;
            hndl->proc = NULL;
        }
        return 0;
    }

    if(0 == fcmd_str2inst(str, &inst, &attr, &proc)){
        hndl->func = -1;
        hndl->inst = inst;
        if(attr != NULL){
            hndl->type = 11;
            hndl->attr = attr;
            hndl->proc = NULL;
        }else if(proc != NULL){
            hndl->type = 12;
            hndl->attr = NULL;
            hndl->proc = proc;
        }else{
            hndl->type = 10;
            hndl->attr = NULL;
            hndl->proc = NULL;
        }
        return 0;
    }

    wc = 0;
    do{
        w[wc] = strtok_r(str, ".", &ctx);
        str = NULL;//Setting str to NULL, after first strtok_r call 
        if(w[wc] == NULL) break;
        wc++;
    }while(1);

    func = fw_fobj_id(w[0]);
    if(func < 0){
        return -1;
    }
    
    if(wc > 1){
        inst = fw_fobj_inst(w[0], w[1]);
        if(inst == NULL){
            if(wc > 2){
                return -1;
            }else{
                if(w[1][0] == '@'){
                    type = 1;
                    attr = &w[1][1];
                }else{
                    type = 2;
                    proc = w[1];
                }
            }
        }else{
            if(wc > 2){
                if(w[2][0] == '@'){
                    type = 11;
                    attr = &w[2][1];
                }else{
                    type = 12;
                    proc = w[2];
                }
            }else{
                type = 10;
            }
        }
    }else{
        type = 0;
    }

    hndl->type = type;
    hndl->func = func;
    hndl->inst = inst;
    hndl->attr = attr;
    hndl->proc = proc;
    
    return 0;
}

static int fcmd_kill(fw_intf_sess_t *sess)
{
    fw_intf_exit();
    sess->resp_result = 0;
    sess->resp_len    = 0;
    
    return 0;
}

static int fcmd_func_list(fw_intf_sess_t *sess)
{
    int func_id;
    int off, lft;
    int ret;
    
    off = 0;
    lft = INTF_BUF_SIZE;
    func_id = 0;

    ret = snprintf((char *)((char *)sess->resp_buf + off), lft, "{");
    off += ret;
    lft -= ret;
    
    while(lft > 0){
        fw_fobj_finfo_t info;
        char fobj_str[32];

        if(0 != fw_fobj_info4id(func_id, &info)){
            break;
        }
        fcmd_fobj2str(func_id, fobj_str, 32);
        if(func_id == 0){
            ret = snprintf((char *)((char *)sess->resp_buf + off), lft,
                           "{%s %s}", info.name, fobj_str);
        }else{
            ret = snprintf((char *)((char *)sess->resp_buf + off), lft,
                           " {%s %s}", info.name, fobj_str);
        }
        off += ret;
        lft -= ret;
        func_id++;
    }
    ret = snprintf((char *)((char *)sess->resp_buf + off), lft, "}");
    off += ret;
    lft -= ret;
    
    sess->resp_result = 0;
    sess->resp_len    = off;
    
    return 0;
}

static int fcmd_func_show(fw_intf_sess_t *sess, int func_id, char *func_str)
{
    fw_fobj_finfo_t info;
    char  *resp = (char *)sess->resp_buf;
    int   i;
    int   lft = INTF_BUF_SIZE;
    int   off = 0;
    int   ret;

    if(0 != fw_fobj_info4id(func_id, &info)){
        sess->resp_len = snprintf(resp, INTF_BUF_SIZE,
                        "Error.InvalidFunc(%s)", func_str);
        sess->resp_result = -1;
        return -1;
    }

    //Show func name
    ret = snprintf(resp, lft, "%s ", info.name);
    off += ret;
    lft -= ret;
    
    //Show func attributes
    ret = snprintf(resp + off, lft, "{{");
    off += ret;
    lft -= ret;
    
    for(i = 0; info.func_desc && i < info.func_desc->attr_nr; i++){
        if(i == 0){
            ret = snprintf(resp + off, lft, "%s",
                                info.func_desc->attrs[i].name);
        }else{
            ret = snprintf(resp + off, lft, " %s",
                                info.func_desc->attrs[i].name);
        }
        off += ret;
        lft -= ret;
    }

    ret = snprintf(resp + off, lft, "}");
    off += ret;
    lft -= ret;

    //Show func process
    ret = snprintf(resp + off, lft, " {");
    off += ret;
    lft -= ret;
    
    for(i = 0; info.func_desc && i < info.func_desc->proc_nr; i++){
        if(i == 0){
            ret = snprintf(resp + off, lft, "%s",
                                info.func_desc->procs[i].name);
        }else{
            ret = snprintf(resp + off, lft, " %s",
                                info.func_desc->procs[i].name);
        }
        off += ret;
        lft -= ret;
    }

    ret = snprintf(resp + off, lft, "}");
    off += ret;
    lft -= ret;

    
    //Show inst attributes
    ret = snprintf(resp + off, lft, " {");
    off += ret;
    lft -= ret;
    
    for(i = 0; info.inst_desc && i < info.inst_desc->attr_nr; i++){
        if(i == 0){
            ret = snprintf(resp + off, lft, "%s",
                                info.inst_desc->attrs[i].name);
        }else{
            ret = snprintf(resp + off, lft, " %s",
                                info.inst_desc->attrs[i].name);
        }
        off += ret;
        lft -= ret;
    }

    ret = snprintf(resp + off, lft, "}");
    off += ret;
    lft -= ret;

    //Show func process
    ret = snprintf(resp + off, lft, " {");
    off += ret;
    lft -= ret;
    
    for(i = 0; info.inst_desc && i < info.inst_desc->proc_nr; i++){
        if(i == 0){
            ret = snprintf(resp + off, lft, "%s",
                                info.inst_desc->procs[i].name);
        }else{
            ret = snprintf(resp + off, lft, " %s",
                                info.inst_desc->procs[i].name);
        }
        off += ret;
        lft -= ret;
    }

    ret = snprintf(resp + off, lft, "}}");
    off += ret;
    lft -= ret;

    sess->resp_result = 0;
    sess->resp_len    = off;
    
    return 0;
}

static int fcmd_func_set(fw_intf_sess_t *sess, int func_id, char *attr, char *val)
{
    if(0 != fw_fobj_set(func_id, attr, val)){
        printf("%s fail\n", attr);
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Set(%s)", attr);
        sess->resp_result = -1;
    }else{
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "%s", val);
        sess->resp_result = 0;
    }
    
    return 0;
}

static int fcmd_func_get(fw_intf_sess_t *sess, int func_id, char *attr)
{
    int ret;

    ret = fw_fobj_get(func_id, (char *)attr, (char *)sess->resp_buf, INTF_BUF_SIZE);
    
    if(0 > ret){
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Get(%s)", attr);
        sess->resp_result = -1;
    }else{
        sess->resp_len = ret;
        sess->resp_result = 0;
    }
    
    return 0;
}

static int fcmd_func_proc(fw_intf_sess_t *sess, int func_id, char *proc, int ac, char **av)
{
    int   ret;
    char *buf = (char *)sess->resp_buf;

    ret = fw_fobj_proc(func_id, proc, ac, av, buf, INTF_BUF_SIZE);
    if(0 > ret){
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Proc(%s)", proc);
        sess->resp_result = -1;
    }else{
        sess->resp_len = ret;
        sess->resp_result = 0;
    }
    return 0;
}

static int fcmd_inst_list(fw_intf_sess_t *sess)
{
    int    ac = sess->req_argc;
    char **av = sess->req_argv;
    
    char  *resp = (char *)sess->resp_buf;
    char  *func_name;
    int   func_id;
    int   inst_id;
    int   off, lft;
    int   ret;

    if(ac < 2){
        sess->resp_result = -1;
        sess->resp_len    = 11;
        strncpy(resp, "Error.Input", INTF_BUF_SIZE);
        return 0;
    }
    func_name = av[1];
    
    func_id = fw_fobj_id(func_name);
    if(func_id < 0){
        sess->resp_result = -1;
        sess->resp_len    = 11;
        strncpy(resp, "Error.Input", INTF_BUF_SIZE);
        return 0;
    }
    
    off = 0;
    lft = INTF_BUF_SIZE;
    inst_id = 0;
    
    ret = snprintf((char *)sess->resp_buf + off, lft, "{");
    off += ret;
    lft -= ret;
    
    while(lft > 0){
        fw_fobj_iinfo_t info;
        char inst_str[32];
        
        
        if(0 != fw_fobj_inst_info4id(func_id, inst_id, &info)){
            break;
        }

        fcmd_inst2str(info.inst, inst_str, 32);
        if(0 == inst_id){
            ret = snprintf(resp + off, lft, "{%s %s}", info.name, inst_str);
        }else{
            ret = snprintf(resp + off, lft, " {%s %s}", info.name, inst_str);
        }
        if(0 > ret){
            break;
        }

        off += ret;
        lft -= ret;
        inst_id++;
    }

    ret = snprintf((char *)sess->resp_buf + off, lft, "}");
    off += ret;
    lft -= ret;
    
    sess->resp_result = 0;
    sess->resp_len    = off;

    return 0;
}

static int fcmd_inst_create(fw_intf_sess_t *sess)
{
    int    ac = sess->req_argc;
    char **av = sess->req_argv;
    int    create_argc;
    char **create_argv;
    char  *func_name;
    char  *inst_name;
    void  *inst;
    char  *resp = (char *)sess->resp_buf;
    
    if(ac < 3){
        sess->resp_result = -1;
        sess->resp_len    = 11;
        strncpy(resp, "Error.Input", INTF_BUF_SIZE);
        return 0;
    }
    func_name = av[1];
    inst_name = av[2];
    create_argc = ac - 3;
    create_argv = &av[3];

    inst = fw_fobj_inst_create(func_name, inst_name,
                               create_argc, create_argv);
    if(inst == NULL){
        sess->resp_result = -1;
        sess->resp_len    = 13;
        strncpy(resp, "Error.Unknown", INTF_BUF_SIZE);
    }else{
        sess->resp_result = 0;
        sess->resp_len    = fcmd_inst2str(inst, resp, INTF_BUF_SIZE);
    }
    
    return 0;
}

static int fcmd_inst_destory(fw_intf_sess_t *sess)
{
    int    ac = sess->req_argc;
    char **av = sess->req_argv;
    char  *func_name;
    char  *inst_name;
    void  *inst;
    char  *resp = (char *)sess->resp_buf;
    
    if(ac < 3){
        sess->resp_result = -1;
        sess->resp_len    = 11;
        strncpy(resp, "Error.Input", INTF_BUF_SIZE);
        return 0;
    }
    func_name = av[1];
    inst_name = av[2];

    inst = fw_fobj_inst(func_name, inst_name);
    if(inst == NULL){
        sess->resp_result = -1;
        sess->resp_len    = 13;
        strncpy(resp, "Error.Input", INTF_BUF_SIZE);
        return 0;
    }

    if(0 != fw_fobj_inst_destory(inst)){
        sess->resp_result = -1;
        sess->resp_len    = 13;
        strncpy(resp, "Error.Unknown", INTF_BUF_SIZE);
    }else{
        sess->resp_result = 0;
        sess->resp_len    = 2;
        strncpy(resp, "Ok", INTF_BUF_SIZE);
    }
    
    return 0;
}

static int fcmd_inst_show(fw_intf_sess_t *sess, void *inst, const char *inst_str)
{
    fw_fobj_iinfo_t info;
    char  *resp = (char *)sess->resp_buf;
    int   i;
    int   lft = INTF_BUF_SIZE;
    int   off = 0;
    int   ret;

    if(0 != fw_fobj_inst_info(inst, &info)){
        sess->resp_len = snprintf(resp, INTF_BUF_SIZE,
                        "Error.InvalidInst(%s)", inst_str);
        sess->resp_result = -1;
        return -1;
    }

    //Show inst name
    ret = snprintf(resp, lft, "%s:%s ", info.base, info.name);
    off += ret;
    lft -= ret;
    
    //Show inst attributes
    ret = snprintf(resp + off, lft, " {{ ");
    off += ret;
    lft -= ret;
    
    for(i = 0; info.desc && i < info.desc->attr_nr; i++){
        ret = snprintf(resp + off, lft, "%s ", 
                                info.desc->attrs[i].name);
        off += ret;
        lft -= ret;
    }

    ret = snprintf(resp + off, lft, "} ");
    off += ret;
    lft -= ret;

    
    //Show inst process
    ret = snprintf(resp + off, lft, " { ");
    off += ret;
    lft -= ret;
    for(i = 0; info.desc && i < info.desc->proc_nr; i++){
        ret = snprintf(resp + off, lft, "%s ", 
                                info.desc->procs[i].name);
        off += ret;
        lft -= ret;
    }
    ret = snprintf(resp + off, lft, "}} ");
    off += ret;
    lft -= ret;

    sess->resp_result = 0;
    sess->resp_len    = off;
    
    return 0;
}

static int fcmd_inst_set(fw_intf_sess_t *sess, void *inst, char *attr, char *val)
{
    if(0 != fw_fobj_inst_set(inst, attr, val)){
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Set(%s)", attr);
        sess->resp_result = -1;
    }else{
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "%s", val);
        sess->resp_result = 0;
    }
    
    return 0;
}

static int fcmd_inst_get(fw_intf_sess_t *sess, void *inst, char *attr)
{
    int ret;

    ret = fw_fobj_inst_get(inst, attr, (char *)sess->resp_buf, INTF_BUF_SIZE);
    
    if(0 > ret){
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Get(%s)", attr);
        sess->resp_result = -1;
    }else{
        sess->resp_len = ret;
        sess->resp_result = 0;
    }
    
    return 0;
}

static int fcmd_inst_proc(fw_intf_sess_t *sess, void *inst, char *proc, int ac, char **av)
{
    int   ret;
    char *buf = (char *)sess->resp_buf;

    ret = fw_fobj_inst_proc(inst, proc, ac, av, buf, INTF_BUF_SIZE);
    if(0 > ret){
        sess->resp_len = snprintf((char *)sess->resp_buf, INTF_BUF_SIZE, "Error.Proc(%s)", proc);
        sess->resp_result = -1;
    }else{
        sess->resp_len = ret;
        sess->resp_result = 0;
    }
    return 0;
}

static int fcmd_hndl_fail(fw_intf_sess_t *sess)
{
    char  *resp = (char *)sess->resp_buf;
    
    sess->resp_len = snprintf(resp, INTF_BUF_SIZE,
                        "Error.InvalidHndl(%s)", sess->req_argv[0]);
    sess->resp_result = -1;
    
    return 0;
}

static int fcmd_syntax_fail(fw_intf_sess_t *sess)
{
    char  *resp = (char *)sess->resp_buf;
    
    sess->resp_len = snprintf(resp, INTF_BUF_SIZE,
                        "Error.InvalidSyntax(%s)", sess->req_line);
    sess->resp_result = -1;
    
    return 0;
}

static int fcmd_hndl(fw_intf_sess_t *sess)
{
    int    ac = sess->req_argc - 1;
    char **av = &sess->req_argv[1];
    char  *resp = (char *)sess->resp_buf;
    fcmd_hndl_t hndl;
    char  *hndl_str;
    
    hndl_str = sess->req_argv[0];
    if(0 != fcmd_str2hndl(hndl_str, &hndl)){
        return fcmd_hndl_fail(sess);
    }

    switch(hndl.type){
        case 0:
            return fcmd_func_show(sess, hndl.func, (char *)"func");
        case 1:
            if(ac == 0){
                return fcmd_func_get(sess, hndl.func, hndl.attr); 
            }else{
                char *setval;
                
                if(ac == 1){
                    setval = av[0];
                }else{
                    if(strcmp(av[0], "=") == 0){
                        setval = av[1];
                    }else{
                        setval = NULL;
                    }
                }
                if(setval){
                    return fcmd_func_set(sess, hndl.func, hndl.attr, setval);
                }else{
                    return fcmd_syntax_fail(sess);
                }
            }
        case 2:
            return fcmd_func_proc(sess, hndl.func, hndl.proc, ac, av);
        case 10:
            return fcmd_inst_show(sess, hndl.inst, (const char *)"inst");
        case 11:
            if(ac == 0){
                return fcmd_inst_get(sess, hndl.inst, hndl.attr); 
            }else{
                char *setval;
                
                if(ac == 1){
                    setval = av[0];
                }else{
                    if(strcmp(av[0], "=") == 0){
                        setval = av[1];
                    }else{
                        setval = NULL;
                    }
                }
                if(setval){
                    return fcmd_inst_set(sess, hndl.inst, hndl.attr, setval);
                }else{
                    return fcmd_syntax_fail(sess);
                }
            }
        case 12:
            return fcmd_inst_proc(sess, hndl.inst, hndl.proc, ac, av);
        default:
            sess->resp_len = snprintf(resp, INTF_BUF_SIZE,
                        "Error.InvalidHndl(%s)", hndl_str);
            sess->resp_result = -1;
            return 0;
    }
}

static void fcmd_func_export(void *node_root, int func_id)
{
    void *func_node;
    void *name_node;
    int   inst_max;
    int   inst_id;
    int   attr_max;
    int   attr_id;
    fw_fobj_finfo_t func_info;
    fw_fobj_iinfo_t inst_info;
    
    if(0 != fw_fobj_info4id(func_id, &func_info)){
        return;
    }

    //Add function object node
    func_node = fw_conf_node_add_child(node_root, "func", NULL);
    if(NULL == func_node){
        return;
    }

    //Add function name node
    name_node = fw_conf_node_add_child(func_node, "name", func_info.name);
    if(NULL == name_node){
        fw_conf_node_delete(func_node);
        return;
    }

    printf("export func: %s\n", func_info.name);
    
    if(func_info.func_desc){
        attr_max = func_info.func_desc->attr_nr;
    }else{
        attr_max = 0;
    }
    
    for(attr_id = 0; attr_id < attr_max; attr_id++){
        char  attr_vbuf[256];
        int   attr_vlen = 256;
        void *attr_node;
        const fw_fobj_attr_t *attr_desc;

        attr_desc = &func_info.func_desc->attrs[attr_id];
        if(    attr_desc->get == NULL
            || attr_desc->set == NULL){
            continue;
        }

        attr_vlen = fw_fobj_get(func_id, attr_desc->name, attr_vbuf, attr_vlen);
        if(0 >= attr_vlen){
            continue;
        }

        attr_node = fw_conf_node_add_child(func_node, "attr", NULL);
        if(attr_node == NULL){
            continue;
        }
        //node_print(attr_node);

        fw_conf_node_add_child(attr_node, "name", attr_desc->name);
        fw_conf_node_add_child(attr_node, "value", attr_vbuf);
        printf("save %s.%s = %s\n", func_info.name, attr_desc->name, attr_vbuf);
    }

    if(NULL == func_info.inst_desc){
        return;
    }else{
        attr_max = func_info.inst_desc->attr_nr;
        printf("inst attr num: %d\n", attr_max);
    }
    
    for(inst_id = 0;
        0 == fw_fobj_inst_info4id(func_id, inst_id, &inst_info);
        inst_id++){
        void *inst_node;
        int   attr_id;

        inst_node = fw_conf_node_add_child(func_node, "inst", NULL);
        if(NULL == inst_node){
            break;
        }
        
        name_node = fw_conf_node_add_child(inst_node, "name", inst_info.name);
        if(NULL == name_node){
            fw_conf_node_delete(inst_node);
            break;
        }
        
        for(attr_id = 0; attr_id < attr_max; attr_id++){
            char  attr_vbuf[256];
            int   attr_vlen = 256;
            void *attr_node;
            const fw_fobj_attr_t *attr_desc;

            attr_desc = &inst_info.desc->attrs[attr_id];
            if(    attr_desc->get == NULL
                || attr_desc->set == NULL){
                continue;
            }

            attr_vlen = fw_fobj_inst_get(inst_info.inst, attr_desc->name, attr_vbuf, attr_vlen);
            if(0 >= attr_vlen){
                continue;
            }

            attr_node = fw_conf_node_add_child(inst_node, "attr", NULL);
            if(attr_node == NULL){
                continue;
            }
            //node_print(attr_node);

            fw_conf_node_add_child(attr_node, "name", attr_desc->name);
            fw_conf_node_add_child(attr_node, "value", attr_vbuf);
            printf("save %s.%s = %s\n", func_info.name, attr_desc->name, attr_vbuf);
        }
    }

    return;
}

static int fcmd_inst_import(const char *func_name, void *inst_node)
{
    void *name_node;
    void *attr_node;
    char *inst_name;
    void *inst;
    
    
    name_node = fw_conf_node_child(inst_node, "name");
    inst_name = fw_conf_node_content(name_node);

    inst = fw_fobj_inst_create(func_name, inst_name, 0, NULL);
    if(NULL == inst){
        printf("Create %s.%s fail\n", func_name, inst_name);
        return -1;
    }

    for(attr_node = fw_conf_node_child(inst_node, "attr");
        attr_node != NULL;
        attr_node = fw_conf_node_next(attr_node, "attr")){
        void *valu_node;
        void *name_node;
        char *attr_name;
        char *attr_valu;
        
        name_node = fw_conf_node_child(attr_node, "name");
        valu_node = fw_conf_node_child(attr_node, "value");
        attr_name = fw_conf_node_content(name_node);
        attr_valu = fw_conf_node_content(valu_node);
        
        if(0 > fw_fobj_inst_set(inst, attr_name, attr_valu)){
            printf("Set %s.%s.@%s <- %s fail\n",
                    func_name, inst_name,
                    attr_name, attr_valu);
            return -1;
        }
    }

    return 0;
}

static int fcmd_func_import(void *func_node)
{
    char *func_name;
    void *inst_node;
    void *name_node;
    void *attr_node;
    void *valu_node;
    int   func_id;

    //node_print(func_node);
    name_node = fw_conf_node_child(func_node, "name");
    if(name_node == NULL){
        return -1;
    }
    
    //node_print(name_node);
    func_name = fw_conf_node_content(name_node);
    func_id   = fw_fobj_id(func_name);
    
    for(attr_node = fw_conf_node_child(func_node, "attr");
        attr_node != NULL;
        attr_node = fw_conf_node_next(attr_node, "attr")){
        char *attr_name;
        char *attr_valu;
        
        name_node = fw_conf_node_child(attr_node, "name");
        valu_node = fw_conf_node_child(attr_node, "value");
        attr_name = fw_conf_node_content(name_node);
        attr_valu = fw_conf_node_content(valu_node);

        if(0 > fw_fobj_set(func_id, attr_name, attr_valu)){
            printf("Set %s.@%s <- %s fail\n",
                    func_name, attr_name, attr_valu);
            return -1;
        }
    }

    for(inst_node = fw_conf_node_child(func_node, "inst");
        inst_node != NULL;
        inst_node = fw_conf_node_next(inst_node, "inst")){
        if(0 != fcmd_inst_import(func_name, inst_node)){
            return -1;
        }
    }

    return 0;
}

/******************************************************************************
 * Global variable                                                            *
 *****************************************************************************/


/******************************************************************************
 * Global function                                                            *
 *****************************************************************************/
int fw_fcmd_load(const char *path)
{
    void *root_node;
    void *func_node;

    //printf("load conf: %s\n", conf);
    root_node = fw_conf_load(path);
    if(root_node == NULL){
        perror("fw_conf_load");
        return -1;
    }

    for(func_node = fw_conf_node_child(root_node, "func");
        func_node != NULL;
        func_node = fw_conf_node_next(func_node, "func")){
        if(0 != fcmd_func_import(func_node)){
            return -1;
        }
    }

    return 0;
}

int fw_fcmd_save(const char *path)
{
    int   func_max;
    int   func_id;
    void *node_root;
    int   ret;

    node_root = fw_conf_node_alloc(path, NULL);
    if(node_root == NULL){
        return -1;
    }
    
    func_max = fw_fobj_cnt();
    for(func_id = 0; func_id < func_max; func_id++){
        fcmd_func_export(node_root, func_id);
    }

    ret = fw_conf_save(node_root, path);

    fw_conf_node_free(node_root);

    return ret;
}

int fw_fcmd_handle(fw_intf_sess_t *sess)
{
    char *req = sess->req_argv[0];
    
    if(strcmp(req, "kill") == 0){
        return fcmd_kill(sess);
    }else if(strcmp(req, "flist") == 0){
        return fcmd_func_list(sess);
    }else if(strcmp(req, "ilist") == 0){
        return fcmd_inst_list(sess);
    }else if(strcmp(req, "create") == 0){
        return fcmd_inst_create(sess);
    }else if(strcmp(req, "destory") == 0){
        return fcmd_inst_destory(sess);
    }else{
        return fcmd_hndl(sess);
    }

    return 0;
}

/* End of fw_fcmd.c */

