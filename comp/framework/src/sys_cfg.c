#include <stdio.h>
#include <string.h>

#include <sys_func.h>

typedef struct _cfg_line_t {
    int   type; // 0:comment, 1: name, 2:setting
    char *info;
}cfg_line_t;

typedef struct _save_ctx_t {
    int   type;
    void *hnd;
    int   siz;
    int   off;
}save_ctx_t;

static char       cfg_line_buf[4096];

int sys_cfg_line_parse(char *line, cfg_line_t *cfg_line)
{
    int   stat;
    int   ch;
    char *pos;

    stat = 0;
    pos = line;

    memset(cfg_line, 0, sizeof(cfg_line_t));

    while((ch = *pos) != 0 && stat != -1){
        switch(stat){
            case 0:
                if(ch == '['){
                    stat = 1;
                }else
                if(ch == '#' || ch == '\r' || ch == '\n'){
                    cfg_line->type = 0;
                    cfg_line->info = pos;
                    goto RET_OK;
                }else
                if(ch == ' ' || ch == '\t'){
                    ;
                }else{
                    cfg_line->type = 2;
                    cfg_line->info = pos;
                    stat = 4;
                }
                break;
            case 1:
                if(ch == ' ' || ch == '\t'){
                    ;
                }else if(ch == ']'){
                    goto RET_FAIL;
                }else{
                    cfg_line->type = 1;
                    cfg_line->info = pos;
                    stat = 2;
                }
                break;
            case 2:
                if(ch == ' ' || ch == '\t'){
                    *pos = 0;
                    stat = 3;
                }else
                if(ch == ']'){
                    *pos = 0;
                    goto RET_OK;
                }
                break;
            case 3:
                if(ch == ']'){
                    *pos = 0;
                    goto RET_OK;
                }
                break;
            case 4:
                if(ch == '#' || ch == '\n' || ch == '\r'){
                    *pos = 0;
                    goto RET_OK;
                }
                break;
            default:
                goto RET_FAIL;
        }
        
        pos++;
    }
    
    if(stat == 4){
        goto RET_OK;
    }
    
RET_FAIL:
    return -1;
    
RET_OK:
    return 0;
}

static void line_save(save_ctx_t *ctx, char *line)
{
    if(ctx->type == 0){
        FILE *fd = ctx->hnd;

        fputs(line, fd);
    }else if(ctx->type == 1){
        char *buf;
        int   lft;
        int   i;

        buf = (char *)(ctx->hnd) + ctx->off;
        lft = ctx->siz - ctx->off;
        for(i = 0; i < lft; i++){
            char ch = line[i];

            if(ch == 0){
                ctx->off += i;
                break;
            }

            buf[i] = ch;
        }
    }
}

static int func_save(void *entry, void *uarg)
{
    save_ctx_t *ctx = (save_ctx_t *)uarg;
    int   max;
    int   len;
    int   i;

    if(sys_func_type(entry) == SYS_FUNC_NULL){
        return 0;
    }

    len = sys_func_entry_get_name(entry, 1024, &cfg_line_buf[1]);
    cfg_line_buf[0] = '[';
    cfg_line_buf[1 + len] = ']';
    cfg_line_buf[2 + len] = '\n';
    cfg_line_buf[3 + len] = 0;
    line_save(ctx, cfg_line_buf);

    max = sys_func_capacity(entry);
    for(i = 0; i < max; i++){
        len = sys_func_get_byidx(entry, i, 4096 - 1, cfg_line_buf);

        if(len <= 0){
            continue;
        }
        cfg_line_buf[len] = 0;
        line_save(ctx, cfg_line_buf);
    }

    return 0;
}

int sys_cfg_save(char *entry_path, char *cfg_name)
{
    save_ctx_t ctx;
    FILE *fd;
    void *entry;

    if(entry_path){
        entry = sys_func_get_entry(entry_path);
    }else{
        entry = sys_func_entry_root();
    }

    if(NULL == entry){
        return -1;
    }

    fd = fopen(cfg_name, "w");
    if(fd == NULL){
        return -1;
    }

    ctx.type = 0;
    ctx.hnd  = fd;

    sys_func_entry_scan(entry, func_save, &ctx);

    fclose(fd);
    
    return 0;
}

int sys_cfg_save_mem(char *entry_path, int cfg_size, char *cfg_buf)
{
    save_ctx_t ctx;
    void *entry;

    if(entry_path){
        entry = sys_func_get_entry(entry_path);
    }else{
        entry = sys_func_entry_root();
    }

    if(NULL == entry){
        return -1;
    }

    ctx.type = 1;
    ctx.hnd  = cfg_buf;
    ctx.siz  = cfg_size;
    ctx.off  = 0;
    
    sys_func_entry_scan(entry, func_save, &ctx);
    
    return 0;
}

int sys_cfg_load(char *cfg_name)
{
    FILE *cfg;
    void *entry = NULL;
    char  line[4096];
    cfg_line_t cfg_line;

    cfg = fopen(cfg_name, "r");
    if(cfg == NULL){
        return -1;
    }
    
    while(fgets(line, 4096, cfg)){
        if(sys_cfg_line_parse(line, &cfg_line) != 0){
            printf("parse fail\n");
            entry = NULL;
            continue;
        }

        if(cfg_line.type == 0){
            continue;
        }else
        if(cfg_line.type == 1){
            entry = sys_func_get_entry(cfg_line.info);
        }else
        if(cfg_line.type == 2){
            if(entry){
                //don't care result
                sys_func_set(entry, strlen(cfg_line.info), cfg_line.info);
            }
        }else{
            //assert(0);
            continue;
        }
    }

    fclose(cfg);
    
    return 0;
}

int sys_cfg_load_mem(int size, char *cfg_buf)
{
    void *entry = NULL;
    char *line  = NULL;
    cfg_line_t cfg_line;
    char *ctx;

    cfg_buf[size] = 0;

    line = strtok_r(cfg_buf, "\n", &ctx);
    for(; line; line = strtok_r(NULL, "\n", &ctx)){
        //printf("line: %s\n", line);
        if(sys_cfg_line_parse(line, &cfg_line) != 0){
            //printf("parse fail\n");
            entry = NULL;
            continue;
        }

        if(cfg_line.type == 0){
            continue;
        }else
        if(cfg_line.type == 1){
            //printf("set func: %s\n", cfg_line.info);
            entry = sys_func_get_entry(cfg_line.info);
        }else
        if(cfg_line.type == 2){
            //printf("set : %s", cfg_line.info);
            if(entry){
                //don't care result
                //int ret = 
                sys_func_set(entry, strlen(cfg_line.info), cfg_line.info);
                //printf("%s\n", ret == 0 ? "OK" : "FAIL");
            }
        }else{
            //assert(0);
            continue;
        }
    }
    
    return 0;
}
