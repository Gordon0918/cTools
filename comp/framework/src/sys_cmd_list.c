#include <stdio.h>
#include <sys_cmd_intf.h>
#include <sys_func.h>

typedef struct {
    int    max;
    int    cur;
    int    off;
    int    siz;
    char  *buf;
    char **list;
}list_ctx_t;

static int cmd_list_scan_func(void *entry, void *arg)
{
    list_ctx_t *ctx = (list_ctx_t *)arg;

    char *name_buf = &ctx->buf[ctx->off];
    int   name_siz = ctx->siz - ctx->off;
    int   name_len = 0;

    if(name_siz == 0){
        //skip scan
        return 1;
    }

    name_len = sys_func_entry_get_name(entry, name_siz - 2, name_buf);

    if(name_len < 0){
        //skip scan
        return 1;
    }

    name_buf[name_len++] = '\n';
    
    ctx->off += name_len;
    return 0;
}

static int cmd_list_all_gen(void *arg, int n, char *buf)
{
    list_ctx_t *ctx = (list_ctx_t *)arg;
    void  *entry;
    int    len;
    

    if(ctx->cur == ctx->max){
        return 0;
    }
    ctx->off = 0;
    ctx->siz = n;
    ctx->buf = buf;

    entry = sys_func_entry_root();
    if(0 == sys_func_entry_scan(entry, cmd_list_scan_func, ctx)){
        len = ctx->off;
    }else{
        len = snprintf(buf, n, "memory not enough\n");
    }

    ctx->cur++;
    
    return len;
}

static int cmd_list_n_gen(void *arg, int n, char *buf)
{
    list_ctx_t *ctx = (list_ctx_t *)arg;
    char  *target;
    void  *entry;
    int    len;
    
    if(ctx->cur == ctx->max){
        return 0;
    }
    target = ctx->list[ctx->cur];

    entry = sys_func_get_entry(target);
    if(entry == NULL){
        len = snprintf(buf, n, "invalid target: %s\n", target);
    }else{
        ctx->off = 0;
        ctx->siz = n;
        ctx->buf = buf;

        if(0 == sys_func_entry_scan(entry, cmd_list_scan_func, ctx)){
            len = ctx->off;
        }else{
            len = snprintf(buf, n, "memory not enough: %s\n", target);
        }
    }

    ctx->cur++;
    
    return len;
}

static int cmd_list_all(void)
{
    list_ctx_t ctx;

    ctx.max  = 1;
    ctx.cur  = 0;
    ctx.list = NULL;

    return sys_cmd_return_dync(0, cmd_list_all_gen, &ctx);
}

static int cmd_list_n(int n, char **list)
{
    list_ctx_t ctx;

    ctx.max  = n;
    ctx.cur  = 0;
    ctx.list = list;
    
    return sys_cmd_return_dync(0, cmd_list_n_gen, &ctx);
}

int sys_cmd_list(int ac, char **av, int dat_len, char *dat)
{
    if(ac == 1){
        return cmd_list_all();
    }else{
        return cmd_list_n(ac - 1, &av[1]);
    }
}
