/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gshell.c
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <gthread.h>
#include <termios.h>
#include <signal.h>
#include <assert.h>

#include <gshell.h>
/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/
#define GSHELL_ALIGN(v, a)  (((v) >> (a)) << (a))

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
enum _gshell_session_stat_e {
    SESSION_INSPACE     = 0,
    SESSION_INWORD   = 1,
};

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gshell_tok_pool_t {
    int  size;
    int  off;
    char *buf;
}gshell_tok_pool_t;

typedef struct _gshell_tok_t {
    int len;
    int idx;
}gshell_tok_t;

typedef struct _gshell_cmd_t {
    char *              name;
    int                 arg_min;
    int                 arg_max;
    gshell_cmd_proc_t   proc;
    gshell_cmd_usage_t  usage;
    gshell_cmd_check_t  check;
}gshell_cmd_t;

typedef struct _gshell_t {
    gthread_mutex_t     lock;
    int                 cmd_max;
    int                 cmd_cnt;
    gshell_cmd_t       *cmd_tbl;
    gshell_tok_pool_t   tok_pool;
}gshell_t;

typedef struct _gsession_evn_t {
    struct list_head list;
    char            *name;
    char            *val;
    int              size;
    char             buf[];
}gsession_evn_t;


/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/
static struct termios std_tty_ctx;

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static void sig_def_handle(int sig)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &std_tty_ctx);
    _exit(0);
}

static int cln_cbreak(void)
{
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &std_tty_ctx);
}

static int set_cbreak(void)
{
    int fd = STDIN_FILENO;
    struct termios   t;
    struct sigaction sa, prev;

    if(tcgetattr(fd, &t) < 0){
        return -1;
    }

    std_tty_ctx = t;

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_lflag |= ISIG;
    t.c_iflag &= ~ICRNL;
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;

    if(tcsetattr(fd, TCSAFLUSH, &t) < 0){
        return -1;
    }

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sa.sa_handler = sig_def_handle;
    if(sigaction(SIGQUIT, NULL, &prev) == -1){
        return -1;
    }
    if(prev.sa_handler != SIG_IGN){
        if(sigaction(SIGQUIT, &sa, NULL) == -1){
            exit(-1);
        }
    }

    sa.sa_handler = sig_def_handle;
    if(sigaction(SIGINT, NULL, &prev) == -1){
        return -1;
    }
    if(prev.sa_handler != SIG_IGN){
        if(sigaction(SIGINT, &sa, NULL) == -1){
            exit(-1);
        }
    }

    sa.sa_handler = sig_def_handle;
    if(sigaction(SIGTERM, NULL, &prev) == -1){
        return -1;
    }
    if(prev.sa_handler != SIG_IGN){
        if(sigaction(SIGTERM, &sa, NULL) == -1){
            exit(-1);
        }
    }

    setbuf(stdout, NULL);

    return 0;
}

static char* gshell_tok_add(gshell_tok_pool_t *pool, char *string)
{
    int   len  = strlen(string) + 1;
    char *tok;

    if(len > (pool->size - pool->off) || len == 0){
        return NULL;
    }
    tok = pool->buf + pool->off;

    memcpy(tok, string, len);

    pool->off += len;

    return tok;
}

static char * gshell_name_add(gshell_t *gsh, char *string)
{
    gshell_tok_pool_t *pool = &gsh->tok_pool;

    return gshell_tok_add(pool, string);
}

static int gshell_lock(gshell_t *gsh)
{
    return gthread_mutex_lock(&gsh->lock);
}

static int gshell_unlock(gshell_t *gsh)
{
    return gthread_mutex_lock(&gsh->lock);
}

static int   gshell_cmd_exit_proc(gshell_session_t *session, int ac, char **av)
{
    return gshell_session_exit(session);
}

static char *gshell_cmd_exit_usage(int arg_idx)
{
    if(arg_idx == 0){
        return "usage: exit";
    }else{
        //arg usage
        return NULL;
    }
}

static int   gshell_cmd_setenv_proc(gshell_session_t *session, int ac, char **av)
{
    return gshell_session_env_set(session, av[1], av[2]);
}

static char *gshell_cmd_setenv_usage(int arg_idx)
{
    if(arg_idx == 0){
        return "usage: setenv <env_name> <env_value>";
    }else if(arg_idx == 1){
        return "<env name>: ascii string";
    }else if(arg_idx == 2){
        return "<env value>: ascii string";
    }else{
        //arg usage
        return NULL;
    }
}

static int   gshell_cmd_getenv_proc(gshell_session_t *session, int ac, char **av)
{
    char *env_val;

    if(ac < 2){
        struct list_head *env_list;
        __list_for_each(env_list, &session->env_list){
            gsession_evn_t *env = list_entry(env_list, gsession_evn_t, list);
            gshell_session_printf(session, "%.8s = %s\n", env->name, env->val);
        }
    }else{
        env_val = gshell_session_env_get(session, av[1]);
        gshell_session_printf(session, "%.8s = %s\n", av[1], env_val);
    }

    return 0;
}

static char *gshell_cmd_getenv_usage(int arg_idx)
{
    if(arg_idx == 0){
        return "usage: getenv <envname>";
    }else if(arg_idx == 1){
        return "<env name>: ascii string";
    }else{
        //arg usage
        return NULL;
    }
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
void *gshell_create(int cmd_max, int flag)
{
    void     *gshell_buf;
    gshell_t *gshell;
    size_t    hdr_mem_size, tok_mem_size, cmd_mem_size;

    hdr_mem_size = GSHELL_ALIGN(sizeof(gshell_t), 4);
    cmd_mem_size = sizeof(gshell_cmd_t) * cmd_max;
    tok_mem_size = 32 * cmd_max;

    gshell_buf = malloc(hdr_mem_size + cmd_mem_size + tok_mem_size);
    if(gshell_buf == NULL){
        return NULL;
    }

    gshell = (gshell_t *)gshell_buf;
    gthread_mutex_init(&gshell->lock, NULL);
    gshell->cmd_max       = cmd_max;
    gshell->cmd_cnt       = 0;
    gshell->cmd_tbl       = (gshell_cmd_t *)(gshell_buf + hdr_mem_size);
    gshell->tok_pool.size = tok_mem_size;
    gshell->tok_pool.off  = 0;
    gshell->tok_pool.buf  = gshell_buf + hdr_mem_size + cmd_mem_size;

    gshell_cmd_add(gshell, "exit",   0, 0, gshell_cmd_exit_proc,   gshell_cmd_exit_usage,   NULL);
    gshell_cmd_add(gshell, "setenv", 2, 2, gshell_cmd_setenv_proc, gshell_cmd_setenv_usage, NULL);
    gshell_cmd_add(gshell, "getenv", 0, 1, gshell_cmd_getenv_proc, gshell_cmd_getenv_usage, NULL);

    return gshell;
}

int   gshell_delete(void *sh)
{
    free(sh);
    return 0;
}

char *gshell_cmd_usage(void *sh, int cmd_id, int arg_id)
{
    gshell_t     *gsh = (gshell_t *)sh;
    gshell_cmd_t *gcmd;

    if(cmd_id >= gsh->cmd_cnt){
        return "$$$$";
    }

    gcmd = &gsh->cmd_tbl[cmd_id];
    if(arg_id > gcmd->arg_max){
        if(arg_id == gcmd->arg_max + 1){
            return " Complete";
        }else{
            return " Too much argument";
        }
    }

    if(gcmd->usage){
        char *usage = gcmd->usage(arg_id);
        if(usage){
            return usage;
        }else{
            return "????";
        }
    }else{
        return "????";
    }
}

int   gshell_cmd_add(void *sh, char *name,
                      int arg_min, int arg_max,
                      gshell_cmd_proc_t  cmd_proc,
                      gshell_cmd_usage_t cmd_usage,
                      gshell_cmd_check_t cmd_check)
{
    gshell_t *gsh    = (gshell_t *)sh;
    int       retval = -1;

    gshell_lock(gsh);
    if(gsh->cmd_cnt < gsh->cmd_max){
        gshell_cmd_t *gcmd;

        gcmd = &gsh->cmd_tbl[gsh->cmd_cnt];
        gcmd->name = gshell_name_add(gsh, name);
        if(gcmd->name){
            gcmd->arg_min = arg_min;
            gcmd->arg_max = arg_max;
            gcmd->proc    = cmd_proc;
            gcmd->usage   = cmd_usage;
            gcmd->check   = cmd_check;
            gsh->cmd_cnt++;
            retval     = 0;
        }
    }
    gshell_unlock(gsh);

    return retval;
}

int   gshell_cmd_find(void *sh, char *name)
{
    gshell_t *gsh    = (gshell_t *)sh;
    int       retval = -1;
    int       i;

    gshell_lock(gsh);
    for(i = 0; i < gsh->cmd_cnt; i++){
        gshell_cmd_t *gcmd;

        gcmd = &gsh->cmd_tbl[i];

        if(strcmp(gcmd->name, name) == 0){
            retval     = i;
            break;
        }
    }
    gshell_unlock(gsh);

    return retval;
}

static int gshell_session_do_addch(gshell_session_t *session, int ch);

static void gshell_session_his_add(gshell_session_t *session)
{
    char *his_buf;

    if(session->his_nr < SESSION_LINEBUF_MAX){
        session->his_id = session->his_nr++;
    }else{
        session->his_id ++;
        if(session->his_id >= SESSION_LINEBUF_MAX){
            session->his_id = 0;
        }
    }

    his_buf = session->his_line[session->his_id];
    memset(his_buf, 0, SESSION_LINEBUF_SIZE + 1);
    //copy without <CR>
    memcpy(his_buf, session->cmd_line, session->cmd_ch - 1);
}

static void gshell_session_his_load(gshell_session_t *session)
{
    int   err = 0;
    char *his_buf;

    if(session->his_in >= session->his_nr){
        return;
    }
    his_buf = session->his_line[session->his_in];
    while(*his_buf && err == 0){
        err = gshell_session_do_addch(session, *his_buf++);
    }
}

static void gshell_session_cmd_load(gshell_session_t *session, gshell_cmd_t *gcmd)
{
    int   err = 0;
    char *cmd_name = gcmd->name;

    while(*cmd_name && err == 0){
        err = gshell_session_do_addch(session, *cmd_name++);
    }
}

static void gshell_session_cmd_clean(gshell_session_t *session)
{
    gshell_session_putch(session, '\r');
    gshell_session_putch(session, '\n');
}

static void gshell_session_cmd_init(gshell_session_t *session)
{
    int i;

    session->cmd_stat = SESSION_INSPACE;
    session->cmd_id   = -1;
    session->cmd_ch   = 0;
    session->cmd_ac   = 0;
    for(i = 0; i < SESSION_CMDARGS_MAX; i++){
        session->cmd_al[i] = 0;
        session->cmd_av[i] = NULL;
    }
    session->his_in   = session->his_id;

    gshell_session_printf(session, "%s", session->prompt);
}

static void gshell_session_autocmd_init(gshell_session_t *session)
{
    session->autocmd_id  = 0;
    session->autocmd_len = 0;
}

static void gshell_session_do_up(gshell_session_t *session)
{
    gshell_session_cmd_clean(session);
    gshell_session_cmd_init(session);
    gshell_session_his_load(session);

    session->his_in--;
    if(session->his_in < 0){
        session->his_in = session->his_id;
    }
}

static void gshell_session_do_down(gshell_session_t *session)
{
    gshell_session_cmd_clean(session);
    gshell_session_cmd_init(session);
    gshell_session_his_load(session);

    session->his_in++;
    if(session->his_in >= session->his_nr){
        session->his_in = 0;
    }
}

static void gshell_session_do_left(gshell_session_t *session)
{
    gshell_session_cmd_clean(session);
    gshell_session_cmd_init(session);
}

static void gshell_session_do_right(gshell_session_t *session)
{
    //complete cmd
    if(session->cmd_stat == SESSION_INWORD && session->cmd_ac == 0){
        gshell_t     *gsh = (gshell_t *)session->sh;
        gshell_cmd_t *gcmd;
        gshell_cmd_t *gcmd_first = NULL;
        int           i, find = 0;

        if(session->autocmd_len == 0){
            memcpy(session->autocmd_str, session->cmd_av[0],
                                         session->cmd_al[0]);
            session->autocmd_len = session->cmd_al[0];
        }

        for(i = 0; i < gsh->cmd_cnt; i++){
            gcmd = &gsh->cmd_tbl[i];
            if(memcmp(gcmd->name, session->autocmd_str,
                                  session->autocmd_len) == 0){
                find++;
                if(find == 1){
                    gcmd_first = gcmd;
                }

                if(session->autocmd_id < find){
                    gshell_session_cmd_clean(session);
                    gshell_session_cmd_init(session);
                    gshell_session_cmd_load(session, gcmd);
                    //set next complete id
                    break;
                }
            }
        }

        if(find > 0){
            if(session->autocmd_id >= find){
                //overloop
                gshell_session_cmd_clean(session);
                gshell_session_cmd_init(session);
                gshell_session_cmd_load(session, gcmd_first);
                session->autocmd_id = 1;
            }else{
                session->autocmd_id = find;
            }
        }
    }
}

static int  gshell_session_do_argcheck(gshell_session_t *session, int ch)
{
    int  arg_idx;
    int  arg_len;
    char arg_str[SESSION_LINEBUF_SIZE];
    int  cmd_id;
    gshell_t *gsh;

    if(session->cmd_ac < 1){
        return 0;
    }

    arg_idx = session->cmd_ac - 1;
    arg_len = session->cmd_al[arg_idx];
    assert(arg_len < SESSION_LINEBUF_SIZE);

    gsh = (gshell_t *)session->sh;

    memcpy(arg_str, session->cmd_av[arg_idx], arg_len);
    arg_str[arg_len] = 0;

    if(arg_idx == 0){
        //check cmd
        cmd_id = gshell_cmd_find(session->sh, arg_str);
        if(cmd_id < 0){
            gshell_session_seterr(session, GSESS_ERR_INVALIDCMD);
            return -1;
        }
        session->cmd_id = cmd_id;
    }else{
        //check arg
        gshell_cmd_t *gcmd;

        cmd_id = session->cmd_id;
        assert(cmd_id >= 0);

        gcmd = &gsh->cmd_tbl[cmd_id];
        if(gcmd->arg_max < arg_idx){
            gshell_session_seterr(session, GSESS_ERR_TOOMUCHARG);
            return -1;
        }

        if(gcmd->check && 0 != gcmd->check(arg_idx, arg_str)){
            gshell_session_seterr(session, GSESS_ERR_INVALIDARG);
            return -1;
        }
    }

    //line complete
    if(ch == '\r'){
        gshell_cmd_t *gcmd;

        cmd_id = session->cmd_id;
        assert(cmd_id >= 0);

        gcmd = &gsh->cmd_tbl[cmd_id];
        if(gcmd->arg_min > arg_idx){
            gshell_session_seterr(session, GSESS_ERR_TOOFEWARG);
            return -1;
        }
    }

    return 0;
}

static void gshell_session_show_err(gshell_session_t *session)
{
    int i;
    int prompt_len;

    prompt_len = strlen(session->prompt);

    gshell_session_putch(session, '\r');
    gshell_session_putch(session, '\n');
    for(i = 0;i < prompt_len; i++){
        gshell_session_putch(session, '$');
    }
    for(i = 0;i < session->cmd_ch; i++){
        gshell_session_putch(session, ' ');
    }
    gshell_session_putch(session, '^');

    switch(session->last_err){
        case GSESS_ERR_INVALIDCMD:
            gshell_session_printf(session, " Invalid cmd!\n");
            break;
        case GSESS_ERR_INVALIDARG:
            gshell_session_printf(session, " Invalid argument!\n");
            break;
        case GSESS_ERR_TOOMUCHARG:
            gshell_session_printf(session, " Too much argument!\n");
            break;
        case GSESS_ERR_TOOFEWARG:
            gshell_session_printf(session, " Too few argument!\n");
            break;
        default:
            gshell_session_printf(session, " Unknown error!\n");
            break;
    }
}

static void gshell_session_show_usage(gshell_session_t *session, char *usage)
{
    int i;
    int prompt_len;

    prompt_len = strlen(session->prompt);

    gshell_session_putch(session, '\r');
    gshell_session_putch(session, '\n');

    for(i = 0;i < session->cmd_ch + prompt_len; i++){
        gshell_session_putch(session, ' ');
    }
    gshell_session_putch(session, '^');
    gshell_session_printf(session, "\"%s\"\r\n", usage);

    gshell_session_printf(session, "%s", session->prompt);
    for(i = 0;i < session->cmd_ch + prompt_len; i++){
        gshell_session_putch(session, session->cmd_line[i]);
    }
}

static int gshell_session_do_addch(gshell_session_t *session, int ch)
{
    if(session->cmd_ch < SESSION_LINEBUF_SIZE){
        gshell_session_putch(session, ch);

        if(isspace(ch)){
            int err;
            if(session->cmd_stat == SESSION_INWORD){
                session->cmd_ac++;
                session->cmd_stat = SESSION_INSPACE;
                assert(session->cmd_ac < SESSION_CMDARGS_MAX);
                err = gshell_session_do_argcheck(session, ch);
                if(err){
                    gshell_session_show_err(session);
                    return -1;
                }
            }else if(ch == '\r'){
                err = gshell_session_do_argcheck(session, ch);
                if(err){
                    gshell_session_show_err(session);
                    return -1;
                }
            }
        }else{
            if(session->cmd_stat == SESSION_INSPACE){
                session->cmd_av[session->cmd_ac] =
                    &session->cmd_line[session->cmd_ch];
                session->cmd_stat = SESSION_INWORD;
            }
            session->cmd_al[session->cmd_ac]++;
        }
        session->cmd_line[session->cmd_ch++] = ch;
    }else{
        //beep alarm!
    }
    return 0;
}

static void gshell_session_do_cr(gshell_session_t *session)
{
    if(0 != gshell_session_do_addch(session, '\r')){
        return;
    }
    gshell_session_putch(session, '\n');

    if(session->cmd_ac){
        gshell_t     *gsh = session->sh;
        gshell_cmd_t *gcmd;
        int           i;

        assert(session->cmd_id >= 0);
        assert(session->cmd_id < gsh->cmd_cnt);

        gshell_session_his_add(session);

        //adjust args
        for(i = 0; i < session->cmd_ac; i++){
            session->cmd_av[i][session->cmd_al[i]] = 0;
        }

        gshell_lock(gsh);

        gcmd = &gsh->cmd_tbl[session->cmd_id];
        if(gcmd->proc){
            session->last_err = gcmd->proc(session, session->cmd_ac, session->cmd_av);
        }else{
            session->last_err = -1;
        }
        gshell_unlock(gsh);
    }
}

static void gshell_session_do_delch(gshell_session_t *session, int ch)
{
    //unsupport now!

    if( session->cmd_ch > 0 ) {
        session->cmd_ch--;
        if(!isspace(session->cmd_line[session->cmd_ch - 1])) {
            if(session->cmd_stat == SESSION_INSPACE) {
                 session->cmd_ac--;
                 //session->cmd_al[session->cmd_ac]--;
                 session->cmd_stat = SESSION_INWORD;
            } else {
                session->cmd_al[session->cmd_ac]--;
            }
        } else {
            if(session->cmd_stat == SESSION_INWORD) {
                 session->cmd_al[session->cmd_ac]--;
                 session->cmd_av[session->cmd_ac] = NULL;
                 session->cmd_stat = SESSION_INSPACE;
            }
        }

        printf("\033[1D");
        printf("\033[K");

    }

    return 0;


}

static void gshell_session_do_help(gshell_session_t *session)
{
    gshell_t     *gsh = session->sh;
    gshell_cmd_t *gcmd;
    char         *usage;
    int i;

    if(session->cmd_ac == 0){
        if(session->cmd_stat == SESSION_INSPACE){
            //list all cmd
            gshell_session_printf(session, "\r\n");
            for(i = 0; i < gsh->cmd_cnt; i++){
                gcmd = &gsh->cmd_tbl[i];
                usage = gshell_cmd_usage(gsh, i, 0);
                gshell_session_printf(session, "%s\n\t%s\n",
                                                gcmd->name, usage);
            }
        }else{
            //list same prefix cmd
            gshell_session_printf(session, "\r\n");
            for(i = 0; i < gsh->cmd_cnt; i++){
                gcmd = &gsh->cmd_tbl[i];
                if(memcmp(gcmd->name, session->cmd_av[0],
                                      session->cmd_al[0]) == 0){
                    usage = gshell_cmd_usage(gsh, i, 0);
                    gshell_session_printf(session, "%s\n\t%s\n",
                                                gcmd->name, usage);
                }
            }
        }
        gshell_session_printf(session, "%s", session->prompt);
        for(i = 0; i < session->cmd_ch; i++){
            gshell_session_putch(session, session->cmd_line[i]);
        }
    }else{
        int cmd_id = session->cmd_id;
        int arg_id = session->cmd_ac;

        assert(cmd_id >= 0);

        usage = gshell_cmd_usage(gsh, cmd_id, arg_id);

        gshell_session_show_usage(session, usage);
    }
}

static int gshell_session_parse(gshell_session_t *session, void *gsh)
{
    int ch;

    gshell_session_cmd_init(session);
    gshell_session_autocmd_init(session);

    while(1){
        ch = gshell_session_getch(session);

        if(iscntrl(ch)){
            //ESC
            if(ch == 27){
                int nch = gshell_session_getch(session);
                if(nch == 91){
                    nch = gshell_session_getch(session);
                    switch(nch){
                        case 65:
                            gshell_session_do_up(session);
                            gshell_session_autocmd_init(session);
                            break;
                        case 66:
                            gshell_session_do_down(session);
                            gshell_session_autocmd_init(session);
                            break;
                        case 67:
                            gshell_session_do_right(session);
                            break;
                        case 68:
                            gshell_session_do_left(session);
                            gshell_session_autocmd_init(session);
                            break;
                        default:
                            ch = nch;
                            break;
                    }
                }else{
                    ch = nch;
                }
            }else if(ch == 13){
                gshell_session_do_cr(session);
                break;
            }
        }

        if(ch == 127 || ch == 8){
            gshell_session_do_delch(session, ch);
            //gshell_session_autocmd_init(session);
        }

        if(isprint(ch)){
            if(ch == '?'){
                gshell_session_do_help(session);
            }else{
                if(0 != gshell_session_do_addch(session, ch)){
                    break;
                }
                gshell_session_autocmd_init(session);
            }
        }
    }
    return 0;
}

static int gshell_session_init(gshell_session_t *session)
{
    if(!isatty(STDIN_FILENO)){
        gshell_session_printf(session, "stdin is not a tty!\n");
        return -1;
    }

    set_cbreak();

    session->done     = 0;
    session->last_err = 0;
    session->prompt   = "-> ";
    session->getch = NULL;//stdio_getch;
    session->putch = NULL;//stdio_putch;
    session->his_nr   = 0;
    session->his_id   = 0;

    INIT_LIST_HEAD(&session->env_list);

    return 0;
}

static int gshell_session_deinit(gshell_session_t *session)
{
    return cln_cbreak();
}

static gsession_evn_t *gshell_session_env_find(gshell_session_t *session, char *name)
{
    gsession_evn_t   *env_match = NULL;
    struct list_head *env_list;

    __list_for_each(env_list, &session->env_list){
        gsession_evn_t *env = list_entry(env_list, gsession_evn_t, list);
        if(strcmp(env->name, name) == 0){
            env_match = env;
        }
    }

    return env_match;
}

int gshell_session_env_set(gshell_session_t *session, char *name, char *value)
{
    size_t name_size;
    size_t val_size;
    size_t env_size;
    gsession_evn_t   *env;

    name_size = strlen(name) + 1;
    val_size  = strlen(value) + 1;
    env_size  = name_size + val_size;
    if(env_size < 252){
        env_size = 252;
    }

    env = gshell_session_env_find(session, name);
    if(env){
        if(env->size - name_size > val_size){
            strcpy(env->val, value);
            return 0;
        }else{
            list_del(&env->list);
            free(env);
            env = NULL;
        }
    }

    env = malloc(env_size);
    if(env == NULL){
        return -1;
    }
    env->size = env_size;
    env->name = &env->buf[0];
    env->val  = &env->buf[name_size];
    strcpy(env->name, name);
    strcpy(env->val,  value);
    list_add_tail(&env->list, &session->env_list);

    return 0;
}

char *gshell_session_env_get(gshell_session_t *session, char *name)
{
    gsession_evn_t   *env;

    env = gshell_session_env_find(session, name);
    if(env){
        return env->val;
    }else{
        return "";
    }
}

int gshell_session(gshell_session_t *session, void *sh,
                   int (*init)(gshell_session_t *, void *),
                   int (*deinit)(gshell_session_t *, void *),
                   void *arg)
{
    gshell_t *gsh = (gshell_t *)sh;

    session->sh = sh;
    if(0 != gshell_session_init(session)){
        return -1;
    }

    if(init){
        init(session, arg);
    }

    while(!session->done){
        gshell_session_parse(session, gsh);
    }

    if(deinit){
        deinit(session, arg);
    }

    gshell_session_deinit(session);

    return session->last_err;
}

int gshell_session_exit(gshell_session_t *session)
{
    session->done = 1;
    return 0;
}

int gshell_session_putch(gshell_session_t *session, int ch)
{
    return putchar(ch);
}

int gshell_session_getch(gshell_session_t *session)
{
    return getchar();
}

int gshell_session_printf(gshell_session_t *session, const char *fmt, ...)
{
    int     retval;
    va_list args;

    va_start(args, fmt);

    retval = vprintf(fmt, args);

    va_end(args);

    return retval;
}

/* End of gshell.c */

