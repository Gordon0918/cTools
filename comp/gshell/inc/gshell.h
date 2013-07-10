/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gshell.h
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

#ifndef _GSHELL_INC_
#define _GSHELL_INC_

#include <list.h>

#ifdef _cplusplus
extern "C" {
#endif

#if      defined HW_XLR732
#define         be64toh(a)    (a)
#define         htobe64(a)    (a)
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define SESSION_CMDARGS_MAX     16
#define SESSION_LINEBUF_MAX     16
#define SESSION_LINEBUF_SIZE    (1024 - 1)

enum _gshell_session_err_t {
    GSESS_ERR_NONE       = 0,
    GSESS_ERR_INVALIDCMD = 1,
    GSESS_ERR_INVALIDARG = 2,
    GSESS_ERR_TOOMUCHARG = 3,
    GSESS_ERR_TOOFEWARG  = 4,
};

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gshell_session_t {
    int   done;
    int   last_err;
    char *prompt;
    void *sh;
    struct list_head env_list;
    int (*getch)(void);
    int (*putch)(int );
    int   cmd_stat;
    int   cmd_id;
    int   cmd_ac;
    int   cmd_al[SESSION_CMDARGS_MAX];
    char *cmd_av[SESSION_CMDARGS_MAX];
    int   cmd_ch;
    char  cmd_line[SESSION_LINEBUF_SIZE + 1];
    int   autocmd_id;
    int   autocmd_len;
    char  autocmd_str[64];
    int   his_nr;
    int   his_id;
    int   his_in;
    char  his_line[SESSION_LINEBUF_MAX][SESSION_LINEBUF_SIZE + 1];
}gshell_session_t;

typedef int   (*gshell_cmd_proc_t) (gshell_session_t *session, int ac, char **av);
typedef char *(*gshell_cmd_usage_t)(int arg_idx);
typedef int   (*gshell_cmd_check_t)(int arg_idx, char *arg_str);
/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
void *gshell_create(int cmd_max, int flag);
int   gshell_delete(void *sh);

int   gshell_cmd_add (void *sh, char *name,
                      int arg_min, int arg_max,
                      gshell_cmd_proc_t  cmd_proc,
                      gshell_cmd_usage_t cmd_usage,
                      gshell_cmd_check_t cmd_check);
int   gshell_cmd_find(void *sh, char *name);
char *gshell_cmd_usage(void *sh, int cmd_id, int arg_id);

int gshell_session(gshell_session_t *session, void *sh,
                   int (*init)(gshell_session_t *, void *),
                   int (*deinit)(gshell_session_t *, void *),
                   void *arg);

int   gshell_session_exit    (gshell_session_t *session);

int   gshell_session_env_set(gshell_session_t *session, char *name, char *value);
char *gshell_session_env_get(gshell_session_t *session, char *name);

int gshell_session_putch(gshell_session_t *session, int ch);
int gshell_session_getch(gshell_session_t *session);
int gshell_session_printf(gshell_session_t *session, const char *fmt, ...);

#define SH_PRINTF    gshell_session_printf

static inline void gshell_session_seterr(gshell_session_t *session, int err)
{
    session->last_err = err;
}

#ifdef _cplusplus
}
#endif

#endif /* _GSHELL_INC_ */

/* End of gshell.h */

