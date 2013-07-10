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
}list_ctx_t;

static int cmd_info_scan_func(void *entry, void *arg)
{
    list_ctx_t *ctx = (list_ctx_t *)arg;

    char *info_buf = &ctx->buf[ctx->off];
    int   info_lft = ctx->siz - ctx->off;
    int   info_off = 0;
    int   info_len = 0;
    int   i;

    if(sys_func_type(entry) == SYS_FUNC_NULL){
        return 0;
    }

    if(info_lft < 128){
        //skip scan
        return 1;
    }

    *info_buf++ = '[';
    info_off++;
    info_lft--;
    
    info_len = sys_func_entry_get_name(entry, info_lft - 2, info_buf);
    info_lft -= info_len;
    info_off += info_len;
    info_buf  = info_buf + info_len;

    *info_buf++ = ']';
    info_lft -= 1;
    info_off += 1;

    info_len = snprintf(info_buf, info_lft, " VERSION:%d\n", sys_entry_ver(entry));
    info_lft -= info_len;
    info_off += info_len;
    info_buf += info_len;

    for(i = 0; i < 79 && info_lft > 1; i++){
        *info_buf++ = '<';
        info_off++;
        info_lft--;
    }
    *info_buf++ = '\n';
    info_off++;
    info_lft--;

    info_len = sys_func_info(entry, info_lft - 1, info_buf);
    if(info_len < 0){
        //scan gono 
        return 0;
    }
    info_buf += info_len;
    info_off += info_len;
    info_lft -= info_len;

    for(i = 0; i < 79 && info_lft > 1; i++){
        *info_buf++ = '>';
        info_off++;
        info_lft--;
    }
    *info_buf++ = '\n';
    info_off++;
    info_lft--;
    
    ctx->off += info_off;

    return 0;
}

static int cmd_info_all_gen(void *arg, int n, char *buf)
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
    if(0 == sys_func_entry_scan(entry, cmd_info_scan_func, ctx)){
        len = ctx->off;
    }else{
        len = snprintf(buf, n, "memory not enough\n");
    }

    ctx->cur++;
    
    return len;
}

static int cmd_info_n_gen(void *arg, int n, char *buf)
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

        if(0 == sys_func_entry_scan(entry, cmd_info_scan_func, ctx)){
            len = ctx->off;
        }else{
            len = snprintf(buf, n, "memory not enough: %s\n", target);
        }
    }

    ctx->cur++;
    
    return len;
}

static int cmd_info_all(void)
{
    list_ctx_t ctx;

    ctx.max  = 1;
    ctx.cur  = 0;
    ctx.list = NULL;

    return sys_cmd_return_dync(0, cmd_info_all_gen, &ctx);
}

static int cmd_info_n(int n, char **list)
{
    list_ctx_t ctx;

    ctx.max  = n;
    ctx.cur  = 0;
    ctx.list = list;
    
    return sys_cmd_return_dync(0, cmd_info_n_gen, &ctx);
}


int sys_cmd_info(int ac, char **av, int dat_len, char *dat)
{
    if(ac == 1){
        return cmd_info_all();
    }else{
        return cmd_info_n(ac - 1, &av[1]);
    }
}
