#include <stdio.h>
#include <string.h>
#include <sys_cmd_intf.h>
#include <sys_func.h>

typedef struct {
    int    max;
    int    cur;
    int    off;
    int    siz;
    char  *buf;
    char **list;
    void  *entry;
}list_ctx_t;

static int cmd_gen_all(void *arg, int size, char *buf)
{
    int len;
    int off;
    int lft;
    int idx;
    list_ctx_t *ctx = (list_ctx_t *)arg;

    if(ctx->cur >= ctx->max){
        return 0;
    }

    off = 0;
    lft = size;
    idx = ctx->cur++;

    if(idx == 0){
        buf[off] = '[';
        lft -= 1;
        off += 1;
        
        len = sys_func_entry_get_name(ctx->entry, lft, &buf[1]);
        if(len == 0){
            return -1;
        }else{
            lft -= len;
            off += len;
        }

        buf[off++] = ']';
        buf[off++] = '\n';
        lft -= 2;
    }

    len = sys_func_get_byidx(ctx->entry, idx, lft, &buf[off]);
    if(len < 0){
        return -1;
    }

    return len + off;
}

static int cmd_gen_n(void *arg, int size, char *buf)
{
    int len;
    int off;
    int lft;
    int idx;
    list_ctx_t *ctx = (list_ctx_t *)arg;

    if(ctx->cur >= ctx->max){
        return 0;
    }

    off = 0;
    lft = size;
    idx = ctx->cur++;

    if(idx == 0){
        buf[off] = '[';
        lft -= 1;
        off += 1;
        
        len = sys_func_entry_get_name(ctx->entry, lft, &buf[1]);
        if(len == 0){
            return -1;
        }else{
            lft -= len;
            off += len;
        }

        buf[off++] = ']';
        buf[off++] = '\n';
        lft -= 2;
    }

    len = sys_func_get_bykey(ctx->entry, ctx->list[idx], lft, &buf[off]);
    if(len < 0){
        return -1;
    }
    
    return len + off;
}

int sys_cmd_get(int ac, char **av, int dat_len, char *dat)
{
    void *entry;
    int   capacity;
    list_ctx_t ctx;

    if(ac < 2){
        return sys_cmd_return(-1, 0, NULL);
    }
    
    entry = sys_func_get_entry(av[1]);
    if(entry == NULL || SYS_FUNC_NULL == sys_func_type(entry)){
        return sys_cmd_return(-1, strlen("Invalid target"), "Invalid target");
    }
    
    capacity = sys_func_capacity(entry);
    if(capacity < 1){
        return sys_cmd_return(-1, strlen("empty target"), "empty target");
    }

    if(ac == 2){
        ctx.cur = 0;
        ctx.max = capacity;
        ctx.list = NULL;
        ctx.entry = entry;
        return sys_cmd_return_dync(0, cmd_gen_all, &ctx);
    }else{
        ctx.cur = 0;
        ctx.max = ac - 2;
        ctx.list = &av[2];
        ctx.entry = entry;
        return sys_cmd_return_dync(0, cmd_gen_n, &ctx);
    }
}